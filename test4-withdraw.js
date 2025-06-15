{
  "TransactionType": "Invoke",
  "Account": "管理者のアドレスをここに入力",
  "Destination": "フックアカウントのアドレスをここに入力",
  "Amount": "3000000",
  "Fee": "2495",
  "Flags": "2147483648",
  "Memos": [
    {
      "Memo": {
        "MemoData": "7769746864726177", // "withdraw" の16進数
        "MemoType": "68757874657874",   // "huxtext" の16進数（例）
        "MemoFormat": "74657874"         // "text" の16進数（例）
      }
    }
  ],
  "DestinationTag": 0,
  "UserWallet": "ユーザ（Alice）の通常Walletアドレスをここに入力"
}
// "Account"は管理者（オペレーター）、"Destination"はフックアカウント、
// "UserWallet"は実際に引き出し先となるAliceのアドレスです。
// 必要に応じてパラメータを調整してください。 