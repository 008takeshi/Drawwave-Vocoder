# Drawable Vocoder — VSTi 仕様書

> マウスで波形・カーブを描いてキャラクターを作り込むボコーダー音源プラグイン

---

## 1. プロダクト概要

### 1.1 コンセプト
「描けるボコーダー」。キャリア波形・バンドゲイン・時間変調カーブを、すべてマウスで描いて作り込めることをコアバリューに据える。既存ボコーダーが「プリセットを選ぶ」ものであるのに対し、本機は「自分で形を作る」ことを前提に UI とシグナルフローを構成する。

### 1.2 ターゲットユーザー
- エレクトロニック/シンセウェーブ系プロデューサー
- 実験音楽・サウンドデザイナー
- 「Daft Punk 的」「Kraftwerk 的」キャラクターを自分の音色で出したい層

### 1.3 差別化ポイント
1. **描けるキャリア波形**(ウェーブテーブル方式)
2. **描けるバンドゲイン**(スペクトル整形を視覚的に)
3. **描ける時間変調**(LFO シェイパー)
4. すべての「描いた形」がプリセットの一部として保存・呼び出し可能

### 1.4 プラグイン形式
- VST3(必須)
- AU(macOS、優先度: 高)
- AAX(将来的、優先度: 低)
- スタンドアロン(デバッグ用)

### 1.5 プラグインタイプ
**インストゥルメント(VSTi)** として実装。MIDI でキャリア(内蔵シンセ)を演奏し、サイドチェイン入力からモジュレーター(声など)を受ける。

---

## 2. シグナルフロー

```
                  ┌──────────────────────────────────┐
[MIDI] ──────────►│ Carrier Section                  │
                  │  ┌──────────┐    ┌─────────────┐ │
                  │  │ 描けるWT │───►│ Polyphonic  │ │── Carrier Out
                  │  │ Oscillator│   │ Voice Engine│ │
                  │  └──────────┘    └─────────────┘ │
                  └──────────────────────────────────┘
                                                    │
                                                    ▼
                  ┌──────────────────────────────────┐
                  │ Vocoder Core (16 band)           │
[Sidechain In] ──►│  ┌─────────┐                     │
   (Modulator)    │  │BPF Bank │─► Envelope Follower │
                  │  │(Analyze)│                   │ │
                  │  └─────────┘                   ▼ │
                  │  ┌─────────┐  ×  ┌──────────────┐│
                  │  │BPF Bank │────►│ Per-Band VCA ││── Vocoded
                  │  │(Synth)  │     │ × Drawn Curve││   Sum
                  │  └─────────┘     └──────────────┘│
                  │  ┌──────────────────────────────┐│
                  │  │ Unvoiced (Sibilance) Detector││
                  │  └──────────────────────────────┘│
                  └──────────────────────────────────┘
                                                    │
                                                    ▼
                  ┌──────────────────────────────────────────────┐
                  │ Output Mix                                   │
                  │  vocoderOut                                  │
                  │  + Mic Mix  × Dry Mic (原音)                 │── Stereo Out
                  │  + Car Mix  × Carrier × Voice Envelope Gate │
                  │  → HP/LP EQ → Master Gain                   │
                  └──────────────────────────────────────────────┘
```

---

## 3. DSP 仕様

### 3.1 サンプリングレート / バッファ
- 対応 SR: 44.1 / 48 / 88.2 / 96 / 192 kHz
- バッファサイズ: 32 ~ 4096 samples、可変対応
- 内部処理は `float`(32bit)

### 3.2 キャリア部(Carrier)

#### 3.2.1 ウェーブテーブル・オシレーター
- 1テーブル = **2048 サンプル**(float 配列)
- 最大 **4 テーブル**を同時保持、X-Y パッドでモーフィング可能(将来拡張で OK、MVP では 1 テーブル固定)
- **アンチエイリアシング**: ミップマップ方式
  - 再生周波数帯ごとに高域カット版を事前生成(オクターブ毎、計 10 段)
  - 起動時 / テーブル編集時にリアルタイム生成(処理時間 < 50ms 目標)
- **補間**: テーブル読み出しは線形補間(MVP)、後に Hermite/4-point に拡張

#### 3.2.2 ボイス・エンジン
- 最大ポリフォニー: **8 ボイス**(設定で 1〜16 に変更可)
- ボイスステアリング: ラウンドロビン + 最古ボイス再利用
- ユニゾン: 1 / 2 / 4 / 8 ボイス、デチューン量(0〜100 cent)、ステレオスプレッド(0〜100%)
- グライド(ポルタメント): 0〜2000 ms、Legato モード対応
- エンベロープ: ADSR(振幅のみ。MVP)
- ピッチベンドレンジ: ±2 semitones(MIDI で変更可)

#### 3.2.3 ノイズソース
- ホワイトノイズを内蔵し、キャリアにミックス可能(0〜100%)
- 子音(s, t, sh)の明瞭度向上に必須

### 3.3 ボコーダーコア

#### 3.3.1 方式
**帯域分割型(アナログ風)** をコアに採用。MVP では FFT 型は実装しない。

#### 3.3.2 バンド構成
- バンド数: **8 / 16 / 24 / 32** から選択(デフォルト 16)
- 周波数レンジ: 80 Hz 〜 11 kHz
- 配置: 対数等間隔(オクターブ単位で均等)
- フィルタ: **4次 Butterworth バンドパス**(Biquad 2段 + cascade)
  - Q 値: 可変(1〜10、デフォルト 4)
  - 解析バンクと合成バンクは同一周波数・同一 Q

#### 3.3.3 エンベロープフォロワー
- 各バンドの解析出力に対し:
  - 全波整流 → 1次ローパス
  - **Attack**: 0.1〜100 ms(デフォルト 5 ms)
  - **Release**: 1〜1000 ms(デフォルト 50 ms)
- Attack / Release はグローバルパラメータ(全バンド共通、MVP)

#### 3.3.4 合成
- 各バンドのキャリアフィルタ出力に、対応する解析側エンベロープを乗算
- **描けるバンドゲインカーブ** が乗算後にもう一段かかる(後述)
- 全バンドを加算 → 出力

#### 3.3.5 無声/有声検出(子音明瞭度)
- モジュレーターのゼロクロス率(ZCR)とスペクトル重心を監視
- 子音(無声)と判定されたフレームでは、キャリアをノイズに切替 or ミックス
- 切替閾値・ミックス量はユーザー調整可能(MVP では ON/OFF + Sensitivity ノブで OK)

### 3.4 描けるパラメータ

#### 3.4.1 キャリア波形(Carrier Waveform)
- 解像度: 2048 サンプル
- 範囲: -1.0 〜 +1.0
- 編集操作: ドロー、消しゴム、スムージング、リセット、反転、正規化、FFT 表示
- プリセット形状: Sine / Saw / Square / Triangle / Pulse / Random
- 再生時はミップマップから補間して読み出し

#### 3.4.2 バンドゲインカーブ(Band Gain Curve)
- 解像度: バンド数と一致(8/16/24/32 点)
- 範囲: 0.0 〜 2.0(0 で完全カット、1.0 でフラット、2.0 で +6dB ブースト)
- 編集操作: ドロー(連続)、ポイント単位編集、スムージング、フラット化、反転
- プリセット: Flat / Tilt Up / Tilt Down / Mid Boost / Smile / Frown / Vowel "A" / "I" / "U" / "E" / "O"
- 母音プリセットは Bark scale ベースのフォルマント近似

#### 3.4.3 LFO シェイパー(Drawable LFO)
- 解像度: 512 サンプル(時間軸)
- 範囲: -1.0 〜 +1.0(双極性)、0.0 〜 1.0(単極性切替可)
- レート: 0.01 Hz 〜 20 Hz、テンポ同期時 1/64 〜 4bar
- 同期: フリーラン / テンポ同期、Retrigger on Note ON 対応
- ルーティング先(MVP では下記から 1 個選択):
  - Formant Shift(±12 semi)
  - Band Gain Tilt(全体を傾ける)
  - Carrier Pitch(±100 cent)
  - Carrier Wavetable Position(将来 4 テーブル化したとき用)
- 編集操作: ドロー、スムージング、リセット、サイン/三角/矩形プリセット呼び出し

### 3.5 出力ステージ
- Wet/Dry ミックス(0〜100%)
- 出力 EQ: ハイパス(20Hz〜500Hz)、ローパス(2kHz〜20kHz)、シェルフ追加は将来
- 出力リミッター: ソフトクリップ(ON/OFF、Threshold -6〜0 dB)
- マスターゲイン: -60 〜 +12 dB

---

## 4. パラメータ一覧

| カテゴリ | パラメータ名 | 範囲 | デフォルト | オートメーション |
|---|---|---|---|---|
| Carrier | Wavetable (描画) | 2048 sample | Saw | × |
| Carrier | Octave | -2〜+2 | 0 | ◯ |
| Carrier | Detune (Unison) | 0〜100 cent | 15 | ◯ |
| Carrier | Unison Voices | 1/2/4/8 | 1 | ◯ |
| Carrier | Glide | 0〜2000 ms | 0 | ◯ |
| Carrier | Noise Mix | 0〜100% | 10 | ◯ |
| Carrier | Amp Attack | 0.1〜2000 ms | 5 | ◯ |
| Carrier | Amp Decay | 1〜2000 ms | 100 | ◯ |
| Carrier | Amp Sustain | 0〜1 | 0.8 | ◯ |
| Carrier | Amp Release | 1〜5000 ms | 200 | ◯ |
| Vocoder | Band Count | 8/16/24/32 | 16 | × |
| Vocoder | Freq Low | 50〜500 Hz | 80 | ◯ |
| Vocoder | Freq High | 4k〜18k Hz | 11000 | ◯ |
| Vocoder | Q | 1〜10 | 4 | ◯ |
| Vocoder | Env Attack | 0.1〜100 ms | 5 | ◯ |
| Vocoder | Env Release | 1〜1000 ms | 50 | ◯ |
| Vocoder | Formant Shift | -12〜+12 semi | 0 | ◯ |
| Vocoder | Band Gain Curve (描画) | 16 points | Flat | × |
| Vocoder | Unvoiced Detect | ON/OFF | ON | ◯ |
| Vocoder | Unvoiced Mix | 0〜100% | 60 | ◯ |
| LFO | Waveform (描画) | 512 sample | Sine | × |
| LFO | Rate | 0.01〜20 Hz | 1 | ◯ |
| LFO | Sync | ON/OFF | OFF | ◯ |
| LFO | Sync Division | 1/64〜4bar | 1/4 | ◯ |
| LFO | Depth | 0〜100% | 50 | ◯ |
| LFO | Destination | enum | Formant | × |
| LFO | Retrigger | ON/OFF | OFF | ◯ |
| Output | Wet/Dry | 0〜100% | 100 | ◯ |
| Output | HP Cutoff | 20〜500 Hz | 80 | ◯ |
| Output | LP Cutoff | 2k〜20k Hz | 16000 | ◯ |
| Output | Limiter | ON/OFF | ON | ◯ |
| Output | Limiter Threshold | -6〜0 dB | -1 | ◯ |
| Output | Master Gain | -60〜+12 dB | 0 | ◯ |

---

## 5. UI 仕様

### 5.1 ウィンドウサイズ
- デフォルト: 1100 × 650 px
- リサイズ: 800×500 〜 1600×950(縦横比固定)
- DPI スケーリング対応(JUCE 標準で OK)

### 5.2 画面構成

```
┌────────────────────────────────────────────────────────────────┐
│ [Logo]     Preset: [Default          ▼]   [A][B]  [Save] [⚙]  │  ← ヘッダー
├────────────────────────────────────────────────────────────────┤
│  ┌──────────────────────┐  ┌─────────────────────────────────┐ │
│  │ CARRIER WAVEFORM     │  │ BAND GAIN CURVE                 │ │
│  │  ┌────────────────┐  │  │  ┌─────────────────────────┐    │ │
│  │  │   描画エリア   │  │  │  │   描画エリア             │    │ │
│  │  │  ~~~~~~~~~     │  │  │  │  ▓▓▓▓ ░░░▓▓ ░░          │    │ │
│  │  └────────────────┘  │  │  └─────────────────────────┘    │ │
│  │ [Sine][Saw][Sq][⟲]   │  │ [Flat][A][I][U][⟲]              │ │
│  └──────────────────────┘  └─────────────────────────────────┘ │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │ LFO SHAPER          ┌──Dest──┐ ┌Rate┐ ┌Depth┐ ┌Sync┐      │ │
│  │  ┌──────────────────┴────────┴─┴────┴─┴─────┴─┴────┴───┐  │ │
│  │  │  ~~~~~~~~~~~~~~~~~~                                 │  │ │
│  │  └─────────────────────────────────────────────────────┘  │ │
│  └────────────────────────────────────────────────────────────┘ │
│  ┌─────Carrier─────┐ ┌─────Vocoder─────┐ ┌─────Output──────┐   │
│  │ Octave  Detune  │ │ Bands  Q  Attck │ │ Wet  Master     │   │
│  │  ◯       ◯      │ │  ◯    ◯   ◯    │ │  ◯    ◯         │   │
│  │ Voices  Noise   │ │ Release  Formant│ │ HP    LP        │   │
│  │  ◯       ◯      │ │  ◯       ◯      │ │  ◯    ◯         │   │
│  └─────────────────┘ └─────────────────┘ └─────────────────┘   │
└────────────────────────────────────────────────────────────────┘
```

### 5.3 描画コンポーネントの操作仕様

#### 5.3.1 共通
- 左ドラッグ: 描画(連続点)
- 右クリック: コンテキストメニュー(Reset / Smooth / Invert / Normalize / FFT 表示)
- ダブルクリック: 該当ポイントを 0(または中央値)にリセット
- Shift + ドラッグ: 水平制約(y 値固定)
- Ctrl/Cmd + ドラッグ: 垂直制約(x 値固定)
- マウスホイール: 表示拡大縮小(将来)

#### 5.3.2 キャリア波形エディタ
- グリッド表示(縦 8 分割、横 4 分割)
- ゼロクロス線を強調表示
- 編集後 100ms 後にリビルド(連続編集中はミップマップ再生成を遅延)
- 編集中はバイパス用の simple sine に切替(クリック音回避)

#### 5.3.3 バンドゲインエディタ
- バーグラフ表示(N バンド分)
- ドラッグでバー単位で描画、複数バーをまたぐとスムージング
- リアルタイムで現在の各バンドのレベルを薄く重ねて表示(VU 的)

#### 5.3.4 LFO エディタ
- 再生位置インジケーター(垂直線)が時間と同期して動く
- 1 周期を画面いっぱいに表示

### 5.4 視覚スタイル
- ダークテーマ基調(`#1A1A1F` 背景、`#2A2A30` パネル)
- アクセントカラー: シアン系(`#00D9FF`)
- 描画エリア: 黒ベース + アクセント色のライン
- フォント: Inter / SF Pro 系 sans-serif

---

## 6. プリセット仕様

### 6.1 ファクトリー・プリセット(最低 16 個)
| 名前 | 概要 |
|---|---|
| Init | デフォルト状態 |
| Classic Robot | 16 band, Saw carrier, 強めの Formant 固定 |
| Daft Style | Unison 4, 軽いコーラス的キャリア |
| Whisper Chorus | Noise Mix 高、A母音 Band Gain |
| Talking Pad | Long Release, ゆっくりした LFO で Formant Shift |
| Buzz Lead | Square carrier, 24 band, 高 Q |
| Vintage 70s | 8 band, Triangle carrier, 軽い Detune |
| Choir | Multi voice, "U" Band Gain, LFO で揺らぎ |
| Spectral Wash | 32 band, Long Attack/Release |
| Glitch | LFO で Formant を激しく動かす |
| Bass Vocoder | Octave -1, LP 強め |
| Sci-Fi Voice | 矩形カーブ、Formant 上 |
| Underwater | Band Gain で低域強調 |
| Megaphone | Band Gain で中域強調 |
| Crystal | 高音域 Band Gain ブースト |
| Monster | Octave -2, Formant Shift 下 |

### 6.2 プリセット保存形式
- JSON ファイル(`.dvoc` 拡張子)
- 全パラメータ + 描画データ(波形配列・バンドゲイン配列・LFO 配列)を含む
- バージョン番号フィールド必須(将来の互換性のため)

```json
{
  "version": 1,
  "name": "My Preset",
  "category": "Lead",
  "params": { "...": "..." },
  "carrier_waveform": [/* 2048 floats */],
  "band_gain": [/* 16 floats */],
  "lfo_shape": [/* 512 floats */]
}
```

---

## 7. パフォーマンス要件

- CPU 使用率: Intel i5 第10世代 / Apple M1 で 16 band / 8 voice / 48kHz 動作時 **5% 以下**(1 インスタンス)
- レイテンシー: バッファサイズに依存、追加レイテンシー **0 sample**(MVP・帯域分割型のため)
- ノイズフロア: -90 dBFS 以下
- 全パラメータ変更時のクリックノイズなし(スムージング 5〜20ms)

---

## 8. 技術スタック

- **言語**: C++17
- **フレームワーク**: JUCE 8.x
- **ビルド**: CMake(JUCE 推奨ワークフロー)
- **対応 OS**:
  - Windows 10/11(64bit、x64)
  - macOS 11 以降(Universal: x86_64 + arm64)
- **依存ライブラリ**:
  - JUCE 同梱の `dsp` モジュール(Biquad 等は自前実装)
  - `nlohmann/json` または `juce::var` でプリセット JSON 処理
- **テスト**: GoogleTest(DSP のユニットテスト)、JUCE の `UnitTestRunner` でも可

---

## 9. 開発フェーズ

### Phase 1: 骨組み ✅
- [x] JUCE プロジェクト初期化、CMake 設定
- [x] サイドチェイン入力対応の Bus 構成
- [x] MIDI で鳴る単純な Saw シンセ(キャリアの仮実装)
- [x] スタンドアロンでのデバッグ動線確立

### Phase 2: ボコーダーコア ✅
- [x] Biquad BPF 実装、バンク化(16 band)
- [x] エンベロープフォロワー
- [x] サイドチェイン入力からのモジュレーター取得
- [x] 加算合成 → 出力
- [x] **マイルストーン: ボコーダーとして音が出る**

### Phase 3: 描けるキャリア ✅
- [x] ウェーブテーブル・オシレーター(線形補間、ミップマップ)
- [x] 描画コンポーネント(WaveformEditor)実装
- [x] テーブル編集 → リアルタイム反映
- [x] アンチエイリアシング検証

### Phase 4: 描けるバンドゲイン ✅
- [x] BandGainEditor 実装
- [x] バンド数の動的切替
- [x] 母音プリセット(A/I/U/E/O)

### Phase 5: LFO シェイパー ✅
- [x] LfoShaperEditor 実装
- [x] テンポ同期
- [x] 各 Destination へのルーティング(Formant / Band Tilt / Pitch)
- [x] パラメータ・スムージング全般の見直し

### Phase 6: 仕上げ ✅
- [x] 無声/有声検出
- [x] 出力ステージ(HP/LP EQ)
- [x] プリセット保存・読込
- [x] ファクトリープリセット作成(16種)
- [x] UI 全体の磨き込み(バーチャルキーボード、レベルメーター)
- [x] パフォーマンス・チューニング

---

## 10. クラス設計(主要クラスのみ)

```
PluginProcessor (juce::AudioProcessor)
├── CarrierEngine
│   ├── WavetableOscillator
│   │   └── MipmapTable (アンチエイリアス済みテーブル群)
│   ├── VoiceManager
│   └── NoiseSource
├── VocoderCore
│   ├── FilterBank (analyze)
│   ├── FilterBank (synth)
│   ├── EnvelopeFollower[N]
│   └── UnvoicedDetector
├── ModMatrix
│   └── DrawableLfo
├── OutputStage
│   ├── EQ
│   └── Limiter
└── PresetManager

PluginEditor (juce::AudioProcessorEditor)
├── WaveformEditor      (キャリア波形)
├── BandGainEditor      (バンドゲイン)
├── LfoShaperEditor     (LFO)
├── KnobPanel × 3       (Carrier / Vocoder / Output)
└── PresetBrowser
```

### 主要なカスタムコンポーネントの責務

**`DrawableCurveComponent`(基底クラス)**
- 描画バッファの保持(`std::vector<float>`)
- マウス入力 → バッファ更新
- 表示の描画(`paint()`)
- 編集通知(リスナー or ValueTree)
- スムージング、リセット、反転などの共通操作

**`WaveformEditor` : public `DrawableCurveComponent`**
- 2048 サンプルの周期波形に特化
- ゼロクロス線・グリッド表示
- 編集後の rebuild イベント発火

**`BandGainEditor` : public `DrawableCurveComponent`**
- バンド数 N に応じた点列表示
- バー表示モード

**`LfoShaperEditor` : public `DrawableCurveComponent`**
- 再生位置インジケーター
- テンポ同期表示

---

## 11. リスクと検証事項

| リスク | 対策 |
|---|---|
| ミップマップ生成が重い | バックグラウンドスレッド、編集中は遅延 |
| 高 Q フィルタの自励発振 | Q 上限を 10 に制限、内部クリッパ |
| バンド合算時のクリッピング | バンドあたりゲイン補正、出力リミッター |
| 描画コンポーネントの再描画コスト | 30fps に制限、Dirty region 管理 |
| サイドチェイン未対応 DAW での挙動 | スタンドアロンで第2入力、説明書きを明確に |
| プリセット互換性破壊 | バージョン番号、マイグレーション関数 |

---

## 12. 用語集

- **キャリア**: 音色を決める波形側。本機では MIDI で演奏する内蔵シンセ
- **モジュレーター**: スペクトル形状の提供側。本機ではサイドチェイン入力(声など)
- **バンド**: 周波数帯域。本機では 8〜32 個の対数等間隔バンド
- **ウェーブテーブル**: 1周期分の波形をテーブル化したもの
- **ミップマップ**: 周波数帯ごとに事前計算されたアンチエイリアス済みテーブル群
- **フォルマント**: 声道共鳴による倍音のピーク。母音の区別に重要

---

---

## 13. 実装状態

全フェーズ完了。以下は初期 SPEC から変更・未実装のもの。

| SPEC の記述 | 実装状態 |
|---|---|
| ユニゾン（1/2/4/8 ボイス）、デチューン、ステレオスプレッド | 未実装 |
| グライド（ポルタメント） | 未実装 |
| 出力リミッター（ON/OFF + Threshold） | 未実装（HP/LP EQ のみ） |
| ウェーブテーブルモーフィング（4 テーブル） | 1 テーブル固定 |
| AAX 形式 | 未実装 |
| スタンドアロン（デバッグ用と記載） | 製品レベルで実装（About ダイアログ、セッション保存） |

---

*Document version: 1.1*
