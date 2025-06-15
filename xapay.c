/**
 * XApay Custom Hook - Final Version with separated functions
 *
 * This version distinguishes between 'withdraw' (refunding a user) and
 * 'debit' (paying the operator from a user's balance), providing clear
 * and distinct functionalities as originally intended.
 */
#include "hookapi.h"

// Operation flags
#define OP_UNKNOWN 0
#define OP_RECHARGE 'r'
#define OP_WITHDRAW 'w'
#define OP_DEBIT 'd'

// Error Codes and Messages
#define SUCCESS 0
#define REJECT_GENERAL -1
#define REJECT_INVALID_TX_TYPE -2
#define REJECT_INVALID_MEMO -3
#define REJECT_INVALID_CURRENCY -4
#define REJECT_UNAUTHORIZED -5
#define REJECT_STATE_READ_FAIL -6
#define REJECT_BALANCE_OVERFLOW -7
#define REJECT_INSUFFICIENT_FUNDS -8
#define REJECT_STATE_WRITE_FAIL -9
#define REJECT_EMIT_FAIL -10
#define REJECT_INVALID_DESTINATION -11

#define MSG_PREFIX "XApay: "
#define MSG_ACCEPT_DEFAULT MSG_PREFIX "Accepted (unhandled)."
#define MSG_RECHARGE_SUCCESS MSG_PREFIX "Recharge successful."
#define MSG_WITHDRAW_SUCCESS MSG_PREFIX "Withdrawal (refund) successful."
#define MSG_DEBIT_SUCCESS MSG_PREFIX "Debit (payment) successful."
#define REJ_MSG_INVALID_TX_TYPE MSG_PREFIX "Invalid transaction type for this operation."
#define REJ_MSG_INVALID_MEMO MSG_PREFIX "Invalid or missing memo."
#define REJ_MSG_INVALID_CURRENCY MSG_PREFIX "Invalid currency; only XAH is supported."
#define REJ_MSG_UNAUTHORIZED MSG_PREFIX "Unauthorized sender for this operation."
#define REJ_MSG_STATE_READ_FAIL MSG_PREFIX "Failed to read account state."
#define REJ_MSG_BALANCE_OVERFLOW MSG_PREFIX "Balance overflow."
#define REJ_MSG_INSUFFICIENT_FUNDS MSG_PREFIX "Insufficient balance."
#define REJ_MSG_STATE_WRITE_FAIL MSG_PREFIX "Failed to write account state."
#define REJ_MSG_EMIT_FAIL MSG_PREFIX "Failed to emit transaction."
#define REJ_MSG_INVALID_DESTINATION MSG_PREFIX "Operation requires a valid Destination account."

// Operator Account
unsigned char OPERATOR_ACCID[20] = {
    0x0f, 0x1d, 0x5c, 0x00, 0x9e, 0x04, 0x0e, 0xf7,
    0x8a, 0xbb, 0x31, 0x08, 0x4f, 0xd5, 0x6c, 0xe0,
    0x5e, 0xd2, 0x41, 0xb3
};

/**
 * Main hook entry point
 */
int64_t hook(uint32_t reserved)
{
    _g(1, 10); // Guard for 10 iterations

    // --- 1. Get transaction details ---
    int64_t tx_type = otxn_type();
    uint8_t tx_originator[20];
    if (otxn_field(SBUF(tx_originator), sfAccount) != 20)
        rollback(SBUF("Could not get originator."), REJECT_GENERAL);

    // --- 2. Parse Memo to determine operation ---
    uint8_t op = OP_UNKNOWN;
    uint8_t memo_buf[32];
    int64_t memo_len = 0;

    int64_t memos_slot = otxn_slot(1);
    if (memos_slot >= 0)
    {
        int64_t memo_field = slot_subfield(memos_slot, sfMemos, 2);
        if (memo_field >= 0)
        {
            int64_t memo_array = slot_subarray(memo_field, 0, 3);
            if (memo_array >= 0)
            {
                int64_t memo_obj = slot_subfield(memo_array, sfMemo, 4);
                if (memo_obj >= 0)
                {
                    int64_t memo_data_f = slot_subfield(memo_obj, sfMemoData, 5);
                    if (memo_data_f >= 0)
                        memo_len = slot(SBUF(memo_buf), memo_data_f);
                }
            }
        }
    }

    if (memo_len > 0)
    {
        uint8_t recharge_hex[] = {0x72, 0x65, 0x63, 0x68, 0x61, 0x72, 0x67, 0x65}; // "recharge"
        uint8_t withdraw_hex[] = {0x77, 0x69, 0x74, 0x68, 0x64, 0x72, 0x61, 0x77}; // "withdraw"
        uint8_t debit_hex[] = {0x64, 0x65, 0x62, 0x69, 0x74}; // "debit"
        
        int is_recharge = 0;
        BUFFER_EQUAL_GUARD(is_recharge, memo_buf, memo_len, recharge_hex, sizeof(recharge_hex), 1);
        if (is_recharge) {
            op = OP_RECHARGE;
        } else {
            int is_withdraw = 0;
            BUFFER_EQUAL_GUARD(is_withdraw, memo_buf, memo_len, withdraw_hex, sizeof(withdraw_hex), 1);
            if (is_withdraw) {
                op = OP_WITHDRAW;
            } else {
                int is_debit = 0;
                BUFFER_EQUAL_GUARD(is_debit, memo_buf, memo_len, debit_hex, sizeof(debit_hex), 1);
                if (is_debit) {
                    op = OP_DEBIT;
                }
            }
        }
    }

    // --- 3. Branch logic based on operation ---
    if (op == OP_RECHARGE)
    {
        // --- RECHARGE: Public operation, anyone can call ---
        if (tx_type != ttPAYMENT)
            rollback(SBUF(REJ_MSG_INVALID_TX_TYPE), REJECT_INVALID_TX_TYPE);

        uint8_t amount_buf[9];
        if (otxn_field(SBUF(amount_buf), sfAmount) != 9 || (amount_buf[0] & 0x80) != 0)
            rollback(SBUF(REJ_MSG_INVALID_CURRENCY), REJECT_INVALID_CURRENCY);
        
        uint64_t drops_to_add = UINT64_FROM_BUF(amount_buf + 1);
        uint8_t* user_account_id = tx_originator;

        uint8_t balance_key[32];
        util_sha512h(SBUF(balance_key), SBUF(user_account_id));
        
        uint8_t balance_buf[8];
        int64_t state_len = state(SBUF(balance_buf), SBUF(balance_key));
        if (state_len == DOESNT_EXIST)
            for (int i = 0; GUARD(8), i < 8; ++i) balance_buf[i] = 0;
        else if (state_len < 0)
            rollback(SBUF(REJ_MSG_STATE_READ_FAIL), REJECT_STATE_READ_FAIL);

        uint64_t balance = UINT64_FROM_BUF(balance_buf);
        if (balance > (0xFFFFFFFFFFFFFFFFULL - drops_to_add))
            rollback(SBUF(REJ_MSG_BALANCE_OVERFLOW), REJECT_BALANCE_OVERFLOW);
        balance += drops_to_add;
        UINT64_TO_BUF(balance_buf, balance);

        if (state_set(SBUF(balance_buf), SBUF(balance_key)) < 0)
            rollback(SBUF(REJ_MSG_STATE_WRITE_FAIL), REJECT_STATE_WRITE_FAIL);

        accept(SBUF(MSG_RECHARGE_SUCCESS), SUCCESS);

    }
    else if (op == OP_WITHDRAW || op == OP_DEBIT)
    {
        // --- WITHDRAW or DEBIT: Privileged operation ---
        if (tx_type != ttINVOKE)
            rollback(SBUF(REJ_MSG_INVALID_TX_TYPE), REJECT_INVALID_TX_TYPE);
        
        int is_operator = 0;
        BUFFER_EQUAL(is_operator, tx_originator, OPERATOR_ACCID, 20);
        if (!is_operator)
            rollback(SBUF(REJ_MSG_UNAUTHORIZED), REJECT_UNAUTHORIZED);

        uint8_t destination_account[20];
        if (otxn_field(SBUF(destination_account), sfDestination) != 20)
            rollback(SBUF(REJ_MSG_INVALID_DESTINATION), REJECT_INVALID_DESTINATION);
        uint8_t* user_account_id = destination_account;

        uint8_t amount_buf[9];
        if (otxn_field(SBUF(amount_buf), sfAmount) != 9 || (amount_buf[0] & 0x80) != 0)
            rollback(SBUF(REJ_MSG_INVALID_CURRENCY), REJECT_INVALID_CURRENCY);
        
        uint64_t drops_to_process = UINT64_FROM_BUF(amount_buf + 1);

        uint8_t balance_key[32];
        util_sha512h(SBUF(balance_key), SBUF(user_account_id));
        
        uint8_t balance_buf[8];
        if (state(SBUF(balance_buf), SBUF(balance_key)) < 0)
            rollback(SBUF(REJ_MSG_STATE_READ_FAIL), REJECT_STATE_READ_FAIL);
        
        uint64_t balance = UINT64_FROM_BUF(balance_buf);
        if (balance < drops_to_process)
            rollback(SBUF(REJ_MSG_INSUFFICIENT_FUNDS), REJECT_INSUFFICIENT_FUNDS);
        balance -= drops_to_process;
        UINT64_TO_BUF(balance_buf, balance);

        if (state_set(SBUF(balance_buf), SBUF(balance_key)) < 0)
            rollback(SBUF(REJ_MSG_STATE_WRITE_FAIL), REJECT_STATE_WRITE_FAIL);

        // --- Determine payment recipient and emit transaction ---
        etxn_reserve(1);
        uint8_t tx_buf[PREPARE_PAYMENT_SIMPLE_SIZE];
        uint8_t* recipient_address;

        if (op == OP_WITHDRAW) {
            // For WITHDRAW (refund), recipient is the user themselves
            recipient_address = user_account_id;
        } else { // op == OP_DEBIT
            // For DEBIT (payment), recipient is the operator who initiated the transaction
            recipient_address = tx_originator;
        }

        PREPARE_PAYMENT_SIMPLE(tx_buf, drops_to_process, recipient_address, 0, 0);

        uint8_t emithash[32];
        if (emit(SBUF(emithash), SBUF(tx_buf)) < 0)
            rollback(SBUF(REJ_MSG_EMIT_FAIL), REJECT_EMIT_FAIL);

        if (op == OP_WITHDRAW)
            accept(SBUF(MSG_WITHDRAW_SUCCESS), SUCCESS);
        else
            accept(SBUF(MSG_DEBIT_SUCCESS), SUCCESS);
    }

    accept(SBUF(MSG_ACCEPT_DEFAULT), SUCCESS);
    return 0;
}
