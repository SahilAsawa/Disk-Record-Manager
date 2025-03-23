#ifndef DISK_H
#define DISK_H

// write files

class DiskModel {

    public:
    DiskModel();
    ~DiskModel();
    void read();
    void write();
    void format();
    void checkDisk();
    void defragment();
    void partition();
    void mount();
    void unmount();
    void eject();
    void encrypt();
    void decrypt();
    void backup();
    void restore();
    void clone();
    void resize();
    void recover();
    void scan();
    void repair();
    void optimize();
    void compress();
    void decompress();
    void encryptFile();
    void decryptFile();
    void encryptFolder();
    void decryptFolder();
    void encryptDisk();
    void decryptDisk();
    void encryptPartition();
    void decryptPartition();
    void encryptVolume();
    void decryptVolume();
    void encryptFileSystem();
    void decryptFileSystem();
    void encryptData();
    void decryptData();
    void encryptBackup();
    void decryptBackup();
    void encryptRestore();
    void decryptRestore();
    void encryptClone();
    void decryptClone();
    void encryptResize();
    void decryptResize();
    void encryptRecover();
    void decryptRecover();
    void encryptScan();
    void decryptScan();
    void encryptRepair();
    void decryptRepair();
    void encryptOptimize();
    void decryptOptimize();
    void encryptCompress();
    void decryptCompress();
    void encryptDecompress();
    void decryptDecompress();             
};

#endif