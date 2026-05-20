#include <iostream>
#include <vector>
#include <string>

using namespace std;

// --------------------------------------------------
// TinySafeFS - Vestigial Crash-Safe Filesystem Demo
// --------------------------------------------------
// This is a simplified educational implementation.
// It demonstrates:
// 1. Journaling
// 2. Copy-on-write
// 3. Crash recovery
// 4. Simple repair mechanism
// --------------------------------------------------

const int TOTAL_BLOCKS = 128;
const int BLOCK_SIZE = 32;

// -----------------------------
// Transaction States
// -----------------------------
enum TransactionState {
    EMPTY = 0,
    STARTED = 1,
    DATA_WRITTEN = 2,
    METADATA_UPDATED = 3,
    COMMITTED = 4
};

// -----------------------------
// Block Structure
// -----------------------------
struct Block {
    string data;
    bool used = false;
};

// -----------------------------
// File Entry
// -----------------------------
struct FileEntry {
    int fileID;
    int blockIndex;
    int size;
    bool valid;
};

// -----------------------------
// Journal Entry
// -----------------------------
struct JournalEntry {
    int transactionID;
    int oldBlock;
    int newBlock;
    TransactionState state;
};

// -----------------------------
// TinySafeFS Class
// -----------------------------
class TinySafeFS {

private:

    vector<Block> memory;
    vector<FileEntry> files;

    JournalEntry journal;

    int nextTransactionID;

public:

    TinySafeFS() {

        memory.resize(TOTAL_BLOCKS);

        nextTransactionID = 1;

        journal.transactionID = 0;
        journal.oldBlock = -1;
        journal.newBlock = -1;
        journal.state = EMPTY;
    }

    // -----------------------------
    // Allocate Free Block
    // -----------------------------
    int allocateBlock() {

        for (int i = 0; i < TOTAL_BLOCKS; i++) {

            if (!memory[i].used) {

                memory[i].used = true;
                return i;
            }
        }

        return -1;
    }

    // -----------------------------
    // Free Block
    // -----------------------------
    void freeBlock(int index) {

        if (index >= 0 && index < TOTAL_BLOCKS) {

            memory[index].used = false;
            memory[index].data.clear();
        }
    }

    // -----------------------------
    // Create File
    // -----------------------------
    void createFile(int fileID, string content) {

        int block = allocateBlock();

        if (block == -1) {
            cout << "No free blocks available\n";
            return;
        }

        memory[block].data = content;

        FileEntry file;
        file.fileID = fileID;
        file.blockIndex = block;
        file.size = content.size();
        file.valid = true;

        files.push_back(file);

        cout << "File created in block " << block << "\n";
    }

    // -----------------------------
    // Find File
    // -----------------------------
    FileEntry* findFile(int fileID) {

        for (auto &file : files) {

            if (file.fileID == fileID && file.valid) {
                return &file;
            }
        }

        return nullptr;
    }

    // -----------------------------
    // Read File
    // -----------------------------
    void readFile(int fileID) {

        FileEntry* file = findFile(fileID);

        if (!file) {
            cout << "File not found\n";
            return;
        }

        cout << "File Data: "
             << memory[file->blockIndex].data
             << "\n";
    }

    // -----------------------------
    // Safe Write Operation
    // -----------------------------
    void safeWrite(int fileID,
                   string newData,
                   bool simulateCrash) {

        FileEntry* file = findFile(fileID);

        if (!file) {
            cout << "File not found\n";
            return;
        }

        int oldBlock = file->blockIndex;

        int newBlock = allocateBlock();

        if (newBlock == -1) {
            cout << "No free blocks available\n";
            return;
        }

        // ---------------------------------
        // STEP 1: Start Transaction
        // ---------------------------------

        journal.transactionID = nextTransactionID++;
        journal.oldBlock = oldBlock;
        journal.newBlock = newBlock;
        journal.state = STARTED;

        cout << "Transaction STARTED\n";

        // ---------------------------------
        // STEP 2: Write New Data
        // ---------------------------------

        memory[newBlock].data = newData;

        journal.state = DATA_WRITTEN;

        cout << "New data written to block "
             << newBlock
             << "\n";

        // ---------------------------------
        // Simulated Crash
        // ---------------------------------

        if (simulateCrash) {

            cout << "*** POWER FAILURE OCCURRED ***\n";
            return;
        }

        // ---------------------------------
        // STEP 3: Update Metadata
        // ---------------------------------

        file->blockIndex = newBlock;
        file->size = newData.size();

        journal.state = METADATA_UPDATED;

        cout << "Metadata updated\n";

        // ---------------------------------
        // STEP 4: Commit Transaction
        // ---------------------------------

        journal.state = COMMITTED;

        cout << "Transaction COMMITTED\n";

        // ---------------------------------
        // STEP 5: Free Old Block
        // ---------------------------------

        freeBlock(oldBlock);

        cout << "Old block freed\n";
    }

    // -----------------------------
    // Recovery Function
    // -----------------------------
    void repairFilesystem() {

        cout << "\n--- RECOVERY STARTED ---\n";

        // ---------------------------------
        // If transaction incomplete
        // rollback changes
        // ---------------------------------

        if (journal.state != COMMITTED &&
            journal.state != EMPTY) {

            cout << "Incomplete transaction detected\n";

            cout << "Rolling back changes...\n";

            freeBlock(journal.newBlock);

            cout << "Discarded block "
                 << journal.newBlock
                 << "\n";

            journal.state = EMPTY;

            cout << "Recovery complete\n";
        }
        else {

            cout << "Filesystem consistent\n";
        }
    }

    // -----------------------------
    // Debug View
    // -----------------------------
    void printStatus() {

        cout << "\n--- FILESYSTEM STATUS ---\n";

        for (auto &file : files) {

            if (file.valid) {

                cout << "File ID: " << file.fileID
                     << " | Block: " << file.blockIndex
                     << " | Data: "
                     << memory[file.blockIndex].data
                     << "\n";
            }
        }

        cout << "Journal State: "
             << journal.state
             << "\n";
    }
};

// --------------------------------------------------
// Main Demonstration
// --------------------------------------------------
int main() {

    TinySafeFS fs;

    // ---------------------------------
    // Create Initial File
    // ---------------------------------

    fs.createFile(1, "OLD_DATA");

    fs.printStatus();

    // ---------------------------------
    // Simulate Crash During Write
    // ---------------------------------

    cout << "\n--- STARTING WRITE OPERATION ---\n";

    fs.safeWrite(1,
                 "NEW_DATA_AFTER_UPDATE",
                 true);

    // ---------------------------------
    // System Restart + Recovery
    // ---------------------------------

    cout << "\n--- SYSTEM RESTARTED ---\n";

    fs.repairFilesystem();

    // ---------------------------------
    // Final State
    // ---------------------------------

    fs.printStatus();

    cout << "\n--- FINAL FILE CONTENT ---\n";

    fs.readFile(1);

    return 0;
}
