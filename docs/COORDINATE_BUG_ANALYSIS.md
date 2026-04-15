# LiquidGlass AEプラグイン - 座標追従問題 技術説明書

## 概要
After Effects用のLiquidGlassプラグインにおいて、2つの座標系関連の重大な不具合が存在します。

---

## 不具合1: 調整レイヤー移動時のガラス形状追従失敗

### 現象
- 調整レイヤーにエフェクトを適用後、レイヤーを移動させると
- ガラス形状（屈折効果の輪郭）が元の位置に残ったまま「空を泳ぐ」
- 実際のレイヤー位置とエフェクトの位置が一致しない

### 技術的な原因

アナリティカルSDFモード（デフォルト）で、ガラス形状の中心が**バッファの中心に固定**されている：

```cpp
// LiquidGlass.cpp:1322-1323
ctx->sdfCx    = (PF_FpLong)(width  - 1) * 0.5;  // バッファ中心に固定
ctx->sdfCy    = (PF_FpLong)(height - 1) * 0.5;
```

SmartRenderパスでは`bufOriginX/Y`が正しく計算される（レイヤー座標系におけるバッファのオフセット）が、これをSDF中心の計算に使用していない。

### 該当コード
- SDF中心設定: `LiquidGlass.cpp:1322-1323`
- 正確な`bufOrigin`計算: `LiquidGlass.cpp:1765-1768`

### 期待される動作
- `sdfCx/sdfCy`はレイヤー座標系でのガラス形状中心を指すべき
- バッファ座標 `(x, y)` → レイヤー座標 `(x + bufOriginX, y + bufOriginY)` の変換が必要
- または、レイヤー移動時にSDF中心も相対的に移動する必要がある

---

## 不具合2: 3Dレイヤーでの座標系未対応

### 現象
- レイヤーを3D化するとエフェクトが正しい位置に表示されない
- または全く反映されないように見える

### 技術的な原因

3DレイヤーはSmartFXを使用せず、レガシー`Render()`パスを通る：

```cpp
// LiquidGlass.cpp:1531-1539
ctx.clipLeft = ctx.clipRight = ctx.clipTop = ctx.clipBottom = FALSE;  // 強制FALSE

/* In the legacy Render path (used by 3D layers)... */
ctx.layerW = in_data->width;    // コンポ幅（レイヤー幅ではない）
ctx.layerH = in_data->height;   // コンポ高さ（レイヤー高さではない）
ctx.bufOriginX = 0;  // 強制0 - レイヤー位置追跡なし
ctx.bufOriginY = 0;  // 強制0 - レイヤー位置追跡なし
```

### 該当コード
- レガシーRenderパス: `LiquidGlass.cpp:1478-1558`
- コメントにも「3D layers」として言及: `LiquidGlass.cpp:1533-1535`

### 期待される動作
- `Render()`パスでも、レイヤーの実際の位置に応じた`bufOrigin`計算が必要
- `in_data`内のレイヤー位置情報を使用して正しいオフセットを算出する必要がある
- または、3Dレイヤー用の別の座標追跡ロジックの実装

---

## 補足: アナリティカルSDF vs Chamfer Distanceの選択ロジック

アナリティカルSDFモードは以下条件で有効（`LiquidGlass.cpp:1302`）：
```cpp
PF_Boolean useAnalyticalSDF = (alphaSource == src) && (ctx->surfTension < 0.5);
```

調整レイヤーの場合、デフォルトでアナリティカルSDFが使用されるため、不具合1が顕在化します。

---

## 修正の方向性（実装方針）

### 1. SDF中心計算の修正
- バッファ中心 `(width-1)*0.5` の代わりに、レイヤー座標系での中心を計算
- `bufOriginX/Y` を考慮した座標変換を追加

### 2. レガシーRenderパスの修正
- `in_data`からレイヤーの位置情報を取得
- `bufOriginX/Y` を正しく計算するロジックを追加
- または、3Dレイヤー用の座標補正を追加

### 3. クリッピングフラグの一貫性
- `Render()`パスでもクリッピング判定を正しく行う
- `extent_hint` などを使用してコンポ境界との関係を判定

---

## ファイル情報

**ファイルパス**: `/Users/yamaha4d/Library/Mobile Documents/com~apple~CloudDocs/CascadeProjects/LiquidGlass/LiquidGlass.cpp`

**主要関数**:
- `LG_DoRender()` - メインレンダリング関数 (line ~1258)
- `Render()` - レガシーRenderパス（3Dレイヤー使用）(line ~1478)
- `PreRender()` - SmartFXパスの事前計算 (line ~1596)
- `SmartRender()` - SmartFXパスのレンダリング (line ~1814)
- `LG_ProcessScanline()` - ピクセル処理 (line ~837)
- `LG_RoundedRectSDF()` - 距離場計算 (line ~715)

**関連データ構造**:
- `LG_RenderCtx` - レンダリングコンテキスト (line ~795)
- `LG_PreRenderData` - SmartFX事前計算データ (line ~1565)

---

## 参考: 構造図

```
調整レイヤー移動時:
├─ SmartRenderパスを使用
├─ bufOriginは正しく計算される（PreRender:1731-1768）
├─ しかしアナリティカルSDFでは中心をバッファ中心に固定（1322-1323）
└─ → ガラス形状がレイヤーに追随しない

3Dレイヤー時:
├─ レガシーRenderパスを使用（1533-1553）
├─ bufOriginX/Y = 0強制（1538-1539）
├─ clipフラグ = FALSE強制（1531）
└─ → エフェクトが正しい位置に反映されない
```

---

## 現在のGIT状態

- リポジトリ: `/Users/yamaha4d/Library/Mobile Documents/com~apple~CloudDocs/CascadeProjects/LiquidGlass`
- 初期コミット: `Initial state - before coordinate tracking fix`
- ブランチ: main

修正実装後は `git diff` で差分を確認できます。
