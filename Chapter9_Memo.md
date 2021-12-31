## 機能分類
表示するモデルごとにグラフィックスパイプラインを変更する必要がある
→ ただの△ポリゴンを描画するか、モデルを描画するのかでルートシグネチャの設定が変わるため

### PMD関連の処理
+ PMDファイル読み込み
+ 頂点
+ マテリアル
+ テクスチャ
+ 白・黒・グレーテクスチャ
+ シェーダーは全部で共通
+ ルートシグネチャ
+ グラフィックスパイプライン
+ ワールド行列
+ コンスタントバッファ→どのシェーダーを使うかは描画する対象次第だから
### DirectX周り
+ デバイス生成
+ コマンドリスト、キュー、アロケータ生成
+ スワップチェーン
+ レンダーターゲット
+ バリア
+ フェンス
+ ビュー行列
+ プロジェクション行列

### DirectXとPMDで共有するもの
+ デバイス
+ コマンドリスト
    - モデルのインスタンス毎に命令発行、共通クラスで描画命令一括実行？
    - スワップチェーンのフリップ処理が入るから、インスタンスのDrawメソッドで実行までやらない方が良さそう

### PMDモデル共有
+ ルートシグネチャ→PMDの描画では同様のモノを使用するため
+ グラフィックスパイプライン→上記と同様の理由
### PMDモデル毎
+ PMDファイル読み込み→コンストラクタとかでファイルパス与えて読みだす
+ 頂点
+ マテリアル
+ テクスチャ
+ ワールド行列
+ 白・黒・グレーテクスチャ
+ 

## その他メモ
### shared_ptr
+ リソースがどのshared_ptrオブジェクトがからも参照されなくなると、自動的に解放される。
+ スレッドセーフ：スレッドを跨いでも安全に利用可能
+ https://cpprefjp.github.io/reference/memory/shared_ptr.html

### 無名名前空間
+ 外部ファイルから参照ができなくなる(内部リンゲージを持つ)
+ staticでも同様のことが可能だが、無名名前空間を使用することが推奨されている
+ 無名名前空間を使用したファイル内で名前衝突が起きることもある。その際は、::修飾子で名前解決を使う
+ https://marycore.jp/prog/cpp/unnamed-namespace/
+ ビルド：コンパイラ＋リンク
+ コンパイル：ソースコードを機械語に翻訳
+ リンク：機械語のプログラムの断片を適切に結合する
+ リンカ：リンクを行うプログラムのこと
+ https://gimo.jp/glossary/details/linker.html

### エラー
[D3D12 ERROR: ID3D12Device::CreateGraphicsPipelineState: Root Signature doesn't match Vertex Shader: Shader CBV descriptor range (BaseShaderRegister=0, NumDescriptors=1, RegisterSpace=0) is not fully bound in root signature
 [ STATE_CREATION ERROR #688: CREATEGRAPHICSPIPELINESTATE_VS_ROOT_SIGNATURE_MISMATCH]]
+ CreateGraphicsPipelineStateでE_INVALIDARG One or more arguments are invalidが出る
#### 原因
+ ルートシグネチャの設定がおかしかった
+ 特にディスクリプタレンジ、ルートパラメーター周りは気を付ける


