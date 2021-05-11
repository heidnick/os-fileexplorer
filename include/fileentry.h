#ifndef __FILEENTRY_H_
#define __FILEENTRY_H_

#include <iostream>
#include <string>
#include <vector>

enum DataType : uint8_t {Directory, Executable, Image, Video, Code, Other};

typedef struct FileEntry {
    std::string name;
    DataType type;
} FileEntry;

class FileEntry {
private:
    std::vector<FileEntry*> _file_entries;

public:
    FileEntry(std::string name, DataType type);

    /*uint32_t createProcess();
    void addVariableToProcess(uint32_t pid, std::string var_name, DataType type, uint32_t size, uint32_t address);
    void modifyVariableToProcess(Variable *var, uint32_t new_size, uint32_t new_address);
    void deleteFreeSpace(uint32_t pid, std::string var_name, uint32_t address);
    void print();
    Process* getProcess(uint32_t pid);
    std::vector<Process*> getProcesses();
    void sortVariables(Process *proc);
    void deleteProcess(uint32_t pid);
    void deleteVariable(uint32_t pid, std::string var_name);*/
};

#endif