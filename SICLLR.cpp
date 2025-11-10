#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <iomanip>
#include <utility>

using namespace std;

//function
void linker(const vector<string> inputFiles);
void parseObjectFile(const string &filename);
void writeReport();
void obj2memory();
void writeMemoryImage();
//linker info
string prog_name;//統一的OBJ program name
bool firstfile=true;
bool transferAddExist = false;
int currentAddress,sizeInBytes,nextAddress,transferAddress,beginAddress = 0;
unordered_map<string, int> symbolTable;
unordered_multimap<int, string> EQUModifyTable;
vector< pair<int,string> >integratedObjectCode;//first=address,second=obj code
vector< pair<int,string> >copyObjectCode;
const int memorySize = 0xFFFF + 1; // 模擬 16 位地址空間
vector<string> memory(memorySize, "FF");
//report variables
vector<string> ReadFileList ;
vector<string> ErrorList ;


int main(int argc, char *argv[]) {
    // 輸入與輸出檔案
    //vector<string> inputFile = {"C_Main","C_RDREC","C_WRREC"};
    vector<string> inputFile;
    for(int i=1;i<argc;i++){
       inputFile.push_back(argv[i]); // 測試用 obj 檔案
    }
    linker(inputFile);
    writeMemoryImage();
    writeReport();
    cout << "Linking Loader Completed!" << endl;
    return 0;
}

void linker(const vector<string> inputFiles){
    for (int i = 0; i < inputFiles.size(); i++){
        parseObjectFile(inputFiles[i]);
    }
    if(!transferAddExist){
        ErrorList.push_back("ERROR!! no transfer address in all object file.");
    }
    copyObjectCode = integratedObjectCode;
    //EQU Mrecord
    for (auto it = EQUModifyTable.begin(); it != EQUModifyTable.end(); it++) {
        int add = it->first;//Modification address
        auto range = EQUModifyTable.equal_range(add);
        int nth_Trecord;
        for(int i=0; i< integratedObjectCode.size() ;i++){//在第幾個Trecord
            int byte_number = integratedObjectCode[i].second.length()/2;
            int start = integratedObjectCode[i].first;
            int end = start + byte_number;
            if(end > add && start <= add){//M address位於該Trecord內
                nth_Trecord = i;
                break;
            }
        }
        int offset = add - integratedObjectCode[nth_Trecord].first;
        int length = stoi( it->second.substr(0,2), nullptr, 16);
        int num_hbyte ;
        if(length%2 == 1){
            num_hbyte = offset*2 +1;
        }
        else{
            num_hbyte = offset*2;
        }
        int modidy_add = stoi( integratedObjectCode[nth_Trecord].second.substr(num_hbyte,length), nullptr, 16);
        char PorS = it->second[2];
        string key = it->second.substr(3);
        if(symbolTable.find(key)!= symbolTable.end()){
            if(PorS == '+'){
                modidy_add += symbolTable[key];
            }
            else if(PorS == '-'){
                modidy_add -= symbolTable[key];
            }
            else{
                stringstream ss;
                ss<<"ERROR!! invalid modification operating symbol: '"<< PorS <<"', in Mrecord Address:" <<hex<< it->first <<", Modify:" << it->second;
                ErrorList.push_back(ss.str());
            }
        }
        else{
            stringstream ss;
            ss<<"ERROR!! unresolved symbol reference : '"<< key << "', in Mrecord Address:" <<hex<< it->first <<", Modify:" << it->second;
            ErrorList.push_back(ss.str());
        }
        
        //將modify_add轉hex str後放回
        stringstream ss;
        ss << uppercase << setfill('0') << setw(length) << hex << modidy_add;
		string str_ss = ss.str();
        integratedObjectCode[nth_Trecord].second.replace(num_hbyte,length,str_ss);
        //else error
    }

}
void writeReport(){
    ofstream outfile("REPORT.txt");
    if (!outfile) {
        cerr << "Failed to open REPORT file!" << endl;
        return;
    }
    // 輸出到檔案
	outfile << "Author Name: E24106018 Chen Ping Hung" << endl;


	outfile << "File Condition: " << endl;
    for(int i=0; i<ReadFileList.size();i++){
        outfile << ReadFileList[i] << endl;
    }
    outfile<<endl;

    outfile << "Program Name: " << prog_name << endl;
    outfile << "Transfer Address: " <<hex<< transferAddress << endl;
    outfile << "Current Address: " <<hex<< currentAddress << endl;
    outfile << "Next Address: " <<hex<< nextAddress << endl;
    outfile << "Size In Bytes: " <<hex<< sizeInBytes << endl;
    outfile << "Begin Address: " <<hex<< beginAddress << endl;
    outfile << endl;

    // 輸出 Symbol Table
    outfile << "Symbol Table:" << endl;
    for (const auto& pair : symbolTable) {
        outfile << "  Symbol: " << pair.first << ", Address: " <<hex<< pair.second << endl;
    }
    outfile << endl;

    // 輸出 EQUModifyTable
    outfile << "EQUModifyTable:" << endl;
    for (const auto& pair : EQUModifyTable) {
        outfile << "  Address: " <<hex<< pair.first << ", Modify: " << pair.second << endl;
    }
    outfile << endl;

    // 輸出 Integrated Object Code
    outfile << "Original Integrated Object Code:" << endl;
    for (const auto& pair : copyObjectCode) {
        outfile << "  Address: " <<hex<< pair.first << ", Object Code: " << pair.second << endl;
    }
    outfile<<endl;

    outfile << "Modified Integrated Object Code:" << endl;
    for (const auto& pair : integratedObjectCode) {
        outfile << "  Address: " <<hex<< pair.first << ", Object Code: " << pair.second << endl;
    }
    outfile<<endl;

    if(ErrorList.empty()){
        outfile << "congratulation!! Linking loader seems complete successfully. " << endl;
    }
    else{
        outfile << "Oops!! " << ErrorList.size() << "error detected!" << endl;
        for(int i=0; i<ErrorList.size();i++){
            outfile << ErrorList[i] << endl;
        }
    }
    // 關閉檔案
    outfile.close();
    cout << "Data has been written to REPORT.txt" << endl;
}

// 功能：解析物件檔
void parseObjectFile(const string &filename) {
    ifstream file(filename);
    string line;
    string programName;
    stringstream ss;
	if (!file) {
        ss << "ERROR!! failed to read object file: " << filename ; 
        ErrorList.push_back(ss.str());
    }
    else{
        ss << filename <<" File opened successfully!";
        ReadFileList.push_back(ss.str());
    }
    while (getline(file, line)) {
        if (line[0] == 'H') {
            // Header: 獲取程式名稱和起始位址
            programName = line.substr(1, 6);
            programName.erase(programName.find_last_not_of(" \t\r\n") + 1);
            int startAddress = stoi(line.substr(7, 6), nullptr, 16);
            int programLength = stoi(line.substr(13, 6), nullptr, 16);
            if(startAddress > 0){
                currentAddress = startAddress;
            }
            if(firstfile){
                beginAddress = startAddress;
                firstfile = false;
            }
            currentAddress = nextAddress;
            nextAddress = currentAddress + programLength;
            sizeInBytes += programLength;
            if(symbolTable.find(programName)!= symbolTable.end()){
                stringstream ss;
                ss<<"ERROR!! duplicate symbol define: '"<< programName <<"'";
                ErrorList.push_back(ss.str());
            }
            else{
                symbolTable[programName] = currentAddress;
            }
            
        } else if (line[0] == 'D') {
            // Define: 將符號載入全域符號表
            for (size_t i = 1; i < line.length(); i += 12) {
                string symbol = line.substr(i, 6);
                symbol.erase(symbol.find_last_not_of(" \t\r\n") + 1);
                int address = stoi(line.substr(i + 6, 6), nullptr, 16) + currentAddress;
                if(symbolTable.find(symbol)!= symbolTable.end()){
                    stringstream ss;
                    ss<<"ERROR!! duplicate symbol define: '"<< symbol <<"'";
                    ErrorList.push_back(ss.str());
                }
                else{
                    symbolTable[symbol] = address;
                }
                
            }
        } else if (line[0] == 'T') {
            // Text: 整合物件碼
            int startAddress = stoi(line.substr(1, 6), nullptr, 16) + currentAddress;
            int length = stoi(line.substr(7, 2), nullptr, 16);
            string objectCode = line.substr(9, length * 2);
            integratedObjectCode.push_back(make_pair(startAddress,objectCode));
        } else if (line[0] == 'M') {
            // Modify: 根據符號表修正
            int address = stoi(line.substr(1, 6), nullptr, 16) + currentAddress;
            int length = stoi(line.substr(7, 2), nullptr, 16);
            string symbol;
            if (line.length() >= 16) { // 檢查字串長度是否足夠
                symbol = line.substr(7, 9);
                symbol.erase(symbol.find_last_not_of(" \t\r\n") + 1);
                EQUModifyTable.insert(make_pair(address, symbol));
            } else if (line.length() > 9) { // 如果長度不足，取剩餘部分
                symbol = line.substr(7);
                symbol.erase(symbol.find_last_not_of(" \t\r\n") + 1);
                EQUModifyTable.insert(make_pair(address, symbol));
            }
            //else 一般modification
        }else if (line[0] == 'E' && line.length() > 1) {
            if(!transferAddExist){
                transferAddExist = true;
                prog_name = programName ; 
                transferAddress = stoi(line.substr(1, 6), nullptr, 16);
            }
            else{
                stringstream ss;
                ss<<"ERROR!! more than one transfer address in program name: '"<< prog_name << "' & '" << programName <<"'" ;
                ErrorList.push_back(ss.str());
            }
        }
    }
    file.close();
}

// 功能：生成記憶體影像檔案
void writeMemoryImage() {
    string MemoryImageName = "DEVF2";
    char type = 'I'; // Type 必須為 'I'
    // 確保名稱長度為 6
    if (prog_name.size() < 6) {
        prog_name.append(6 - prog_name.size(), ' ');
    }

    // 創建字串
    stringstream ss;
    ss << type;
    ss << prog_name;
    ss << uppercase << setfill('0') << setw(6) << hex << beginAddress;
    ss << uppercase << setfill('0') << setw(6) << hex << sizeInBytes;
    ss << uppercase << setfill('0') << setw(6) << hex << transferAddress;
    string firstLine = ss.str();
    //obj2memory
    obj2memory();
    ofstream outputFile(MemoryImageName);
    if (!outputFile) {
        stringstream stmp;
        stmp<<"ERROR!! Can't open memory image file: "<<MemoryImageName;
        ErrorList.push_back(stmp.str());
        return;
    }
    outputFile << firstLine << "\n";
    // 每 32 個字串組成一行，輸出到檔案
    const size_t groupSize = 32;
    for (size_t i = 0; i < sizeInBytes; i += groupSize) {
        for (size_t j = i; j < i + groupSize && j < memory.size(); ++j) {
            outputFile << memory[j];
        }
        outputFile << "\n"; // 換行
    }
    //output firstline & memory image
	outputFile.close();
}

void obj2memory(){
    for(int i=0;i<integratedObjectCode.size();i++){
        string objcode = integratedObjectCode[i].second;
        int length = objcode.size()/2;
        int Startadd = integratedObjectCode[i].first;
        for(int j=0;j<length;j++){
            string tmp = objcode.substr(j*2,2);
            memory[Startadd+j] = tmp;
        }
    }
}
