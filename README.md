# xapay-hooks

## 概要

XApay 用の Xahau Hook およびテスト用トランザクション JSON 集です。

- `xapay.c`: XApay Hook 本体
- `test1-charge.js` など: テスト用トランザクション JSON

## 使い方

1. `xapay.c` を Xahau ネットワークにデプロイ
2. テスト用 JSON ファイルを使ってトランザクションを送信
3. フックの動作や残高変化を確認

## 今後の課題

## MEMOS についての注意

- トランザクションの `Memos` フィールドには、操作種別（recharge, withdraw, debit など）を 16 進数で指定しています。
- 例：
  ```json
  "Memos": [
    {
      "Memo": {
        "MemoData": "7265636861726765", // "recharge" の16進数
        "MemoType": "68757874657874",   // "huxtext" の16進数（例）
        "MemoFormat": "74657874"         // "text" の16進数（例）
      }
    }
  ]
  ```
- しかし、現状フック側で MEMO の内容が正しく認識されず、`Accepted (unhandled)` となる問題が解消できていません。
- 送信側の形式は正しいものの、フック（xapay.c）側の MEMO 取得・比較ロジックに課題が残っています。
