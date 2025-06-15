const xrpl = require("xrpl");

async function main() {
  const client = new xrpl.Client("wss://xahau.network");
  await client.connect();

  const account = "rhyYNdxAyFQ7s2KYXhaTMJKF7NrkkZj1X9";

  try {
    // アカウント情報を取得
    const response = await client.request({
      command: "account_info",
      account: account,
      ledger_index: "validated",
    });

    console.log("Account Info:");
    console.log(JSON.stringify(response.result, null, 2));

    // フック情報を取得
    const hookResponse = await client.request({
      command: "account_hooks",
      account: account,
    });

    console.log("\nHook Info:");
    console.log(JSON.stringify(hookResponse.result, null, 2));
  } catch (error) {
    console.error("Error:", error);
  } finally {
    await client.disconnect();
  }
}

main();
