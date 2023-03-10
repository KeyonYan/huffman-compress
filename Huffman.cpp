// Huffman.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <limits>
#include <queue>
#include <fstream>
#include <Windows.h>
#include <WinBase.h>
#include <tchar.h>
#include <string>
#include <bitset>
#include <thread>
#include <io.h>
using namespace std;
//TreeNode
typedef struct TreeNode TreeNode;
struct TreeNode {
	//构造器
	TreeNode() :weight(0), val(), Left(NULL), Right(NULL) {}
	//运算符重载
	bool operator <(const TreeNode &x) const;
	bool operator >(const TreeNode &x) const;
	unsigned char val; //字符
	int weight; //权重
	TreeNode* Left; //左子树
	TreeNode* Right; //右子树
};
bool TreeNode::operator < (const TreeNode &x) const {
	return x.weight < weight;
}
bool TreeNode::operator > (const TreeNode &x) const {
	return x.weight < weight;
}


unsigned long long ByteWeight[256]; //记录字节权重
string HuffmanCodeTable[256]; //保存Huffman编码
priority_queue<TreeNode> PQ; //最小优先队列
string file_path; //需要压缩的文件路径
string filecps_path; //压缩后的文件路径
string file_ucps_path; //解压后的文件路径
int BufPageNum, cPos, dPos; //解码缓冲区的页码数、编码缓冲区当前位置索引、解码缓冲区当前位置索引
bool flagProcess; //进度flag

bool CompressFile(string file_path); //压缩文件
bool GetWeight(); //字节流方式读取将要压缩的文件,获取每个字节的权重
bool InsertPQ(); //获取每种字节出现的次数，计算权重，插入最小优先队列中
void MakeHuffmanTree(TreeNode& RootNode); //构造HuffmanTree
void GetHuffmanCode(TreeNode& Root, char* codeBuf, int nDeep, string HuffmanCodeTable[]); //获取Huffman编码
bool WriteHuffmanCode(); //向压缩文件写入配置信息和Huffman编码后的数据
bool UnCompressFile(string filecps_path); //解压缩文件
LPCWSTR stringToLPCWSTR(std::string str); //string转LPCWSTR
void Decode(HANDLE& cFile, HANDLE& nFile, DWORD& dwWriteNum, DWORD& dwReadNum, TreeNode* Root, unsigned long long ValidMsgSize, string& codeBufstr, unsigned char decodeBuf[], int& decodeBufLen, int& cPos, int& dPos);
void PrintProcessRate(); //进度条打印

int main() {
	system("title HuffmanTree Compress And Uncompress");
	cout << "===>>>基于Huffman树实现文件压缩与解压缩<<<===" << endl << endl
		<< "1.压缩文件" << endl << endl
		<< "2.解压文件" << endl << endl
		<< "请选择：";
	int ch;
	cin >> ch;
	if (ch == 1) {
		cout << "请输入需要压缩的文件名：";
		cin >> file_path;
		//判断文件是否存在
		while (_access(file_path.c_str(), 0) == -1) {
			cout << "文件不存在...请重新输入！" << endl;
			cout << "请输入需要压缩的文件名：";
			cin >> file_path;
		}

		size_t index = file_path.find_first_of('.');
		cout << "请输入压缩后的文件名(后缀.cps)(如果输入符号1，则默认为" << file_path.substr(0,index) << ".cps)：";
		cin >> filecps_path;
		if (filecps_path == string("1")) {
			filecps_path = file_path.substr(0, index) + ".cps";
		}
		else {
			index = filecps_path.find_first_of('.');
			while (filecps_path.substr(index, 4) != ".cps") {
				cout << "文件后缀必须为.cps！请重新输入！" << endl
					<< "请输入压缩后的文件名(后缀.cps)：";
				cin >> filecps_path;
			}
		}
		
		thread watch(PrintProcessRate); //创建watch线程监视进度
		if (CompressFile(file_path)) {
			flagProcess = true;
			watch.join(); //等待watch线程执行完
			cout << "文件压缩完毕..." << endl;

			cout << "--------------------------------------------" << endl
				<< "	  ===>>编码表<<===" << endl
				<< "│ 字节码(DEC)	│ 权重	│ Huffman编码" << endl;
			for (int i = 0; i < 256; i++) {
				if (!HuffmanCodeTable[i].empty())
					cout << "│ " << i << "		│ " << ByteWeight[i] << "	│ " << HuffmanCodeTable[i] << endl;
			}
			for (int i = 0; i < 256; ++i) {
				HuffmanCodeTable[i].clear();
			}
			return 1;
		}
		else {
			watch.join(); //等待watch线程执行完
			cout << "文件压缩失败..." << endl;
			return -1;
		}
	}
	else if (ch == 2) {
		cout << "请输入需要解压的文件名(后缀.cps)：";
		cin >> filecps_path;
		//判断文件是否存在
		while (_access(filecps_path.c_str(), 0) == -1) {
			cout << "文件不存在...请重新输入！" << endl;
			cout << "请输入需要压缩的文件名：";
			cin >> filecps_path;
		}
		size_t index = filecps_path.find_first_of('.');
		while (filecps_path.substr(index, 4) != ".cps") {
			cout << "文件后缀必须为.cps！请重新输入！" << endl
				<< "请输入需要压缩的文件名(后缀.cps)：";
			cin >> filecps_path;
		}
		cout << "请输入解压后的文件名：";
		cin >> file_ucps_path;
		thread watch(PrintProcessRate); //创建watch线程监视进度
		if (UnCompressFile(filecps_path)) {
			flagProcess = true;
			watch.join(); //等待watch线程执行完
			cout << "解压完毕..." << endl;
			return 1;
		}
		else {
			watch.join(); //等待watch线程执行完
			cout << "解压失败..." << endl;
			return -1;
		}
	}
}

void PrintProcessRate() {
	while (!flagProcess) {
		cout << "\r-...";
		Sleep(250);
		cout << "\r/...";
		Sleep(250);
		cout << "\r-...";
		Sleep(250);
		cout << "\r\\...";
		Sleep(250);
	}
	cout << endl;
}

//string转LPCWSTR
LPCWSTR stringToLPCWSTR(std::string str) {
	size_t size = str.length();
	wchar_t* buf = new wchar_t[size + 1];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), size, buf, size*sizeof(wchar_t));
	buf[size] = 0; //End'\0'
	return buf;
}

//压缩文件
bool CompressFile(string file_path) {
	GetWeight(); //字节流方式读取将要压缩的文件,获取每个字节的权重

	InsertPQ(); //将计算后的每个字符的权重，插入最小优先队列中
	
	TreeNode RootNode; //HuffmanTree Root
	MakeHuffmanTree(RootNode);

	char Buf[1024] = { 0 };
	GetHuffmanCode(RootNode, Buf, 0, HuffmanCodeTable); //获取Huffman编码
	
	

	WriteHuffmanCode(); //向压缩文件写入配置信息和Huffman编码后的数据
	
	
	return true;
}

//字节流方式读取将要压缩的文件,获取每个字节的权重
bool GetWeight() {
	//以二进制方式打开文件，以字节流方式读取文件
	unsigned char readBuf[256] = { 0 };
	
	HANDLE hFile = CreateFile(
		stringToLPCWSTR(file_path),
		GENERIC_READ,
		FILE_SHARE_READ,
		0,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_READONLY,
		NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		std::cout << "打开文件(" << file_path << ")失败，请检查文件是否存在" << endl;
		return 0;
	}
	DWORD dwReadNum;
	while (ReadFile(hFile, readBuf, 256, &dwReadNum, NULL)) {
		if (dwReadNum == 0) break;
		for (unsigned int i = 0; i < dwReadNum; ++i) {
			ByteWeight[readBuf[i]]++;
		}
		//清理缓冲区
		memset(readBuf, 0, sizeof(readBuf));
	}
	CloseHandle(hFile);

	//将权重更改为相对权重，存储占用空间小
	unsigned long long tmp[256] = { 0 };
	memcpy(tmp, ByteWeight, sizeof(ByteWeight));
	sort(tmp, tmp + 256);
	bool visited[256] = { 0 };
	bool flag = false;
	unsigned long long reaweight = 1;
	for (int i = 0; i < 256; ++i) {
		if (tmp[i] == 0) continue;
		flag = false;
		for (int j = 0; j < 256; ++j) {
			if (visited[j] == 0 && tmp[i] == ByteWeight[j]) {
				visited[j] = 1;
				ByteWeight[j] = reaweight;
				flag = true;
			}
		}
		if (flag)
			++reaweight;
	}

	return true;
}
//获取每种字符出现的次数，计算权重，插入最小优先队列中
bool InsertPQ() {
	//遍历字符权重数组，只将权重>0的字符，插入最小优先队列中
	TreeNode curNode;
	for (int i = 0; i < 256; ++i) {
		if (ByteWeight[i] != 0) {
			curNode.val = i;
			curNode.weight = ByteWeight[i];
			PQ.push(curNode);
		}
	}
	return true;
}

void MakeHuffmanTree(TreeNode& RootNode) {
	TreeNode child1, child2;
	unsigned long long maxweight = 0;
	while (PQ.size() != 1) {
		child1 = PQ.top();
		PQ.pop();
		child2 = PQ.top();
		PQ.pop();
		RootNode.val = 0;
		RootNode.weight = child1.weight + child2.weight;
		RootNode.Left = new TreeNode;
		*RootNode.Left = child1;
		RootNode.Right = new TreeNode;
		*RootNode.Right = child2;
		PQ.push(RootNode);
	}
}

//获取Huffman编码
void GetHuffmanCode(TreeNode& Root, char* codeBuf, int nDeep, string HuffmanCodeTable[]) {
	if (Root.Left == NULL && Root.Right == NULL) {
		HuffmanCodeTable[Root.val] = codeBuf;
	}
	codeBuf[nDeep] = '0';
	if (Root.Left != NULL)
		GetHuffmanCode(*Root.Left, codeBuf, nDeep + 1, HuffmanCodeTable);
	codeBuf[nDeep] = '1';
	if (Root.Right != NULL)
		GetHuffmanCode(*Root.Right, codeBuf, nDeep + 1, HuffmanCodeTable);
	codeBuf[nDeep] = 0;
}

//向压缩文件写入得到的Huffman编码
bool WriteHuffmanCode() {
	//根据HuffmanCodeTable（下标作为该字节，储存Huffman编码）
	//再次以只读模式打开文件，按字节每读取一次，将其对应的Huffman编码写入另一个文件
	unsigned char readBuf[256] = { 0 };
	HANDLE hFile = CreateFile(
		stringToLPCWSTR(file_path),
		GENERIC_READ,
		FILE_SHARE_READ,
		0,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_READONLY,
		NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		std::cout << "File Open Error: " << file_path << endl;
		return 0;
	}
	/*int pos = file_path.rfind('.',file_path.length());
	string filecps_path = file_path.substr(0,pos+1) + "cps";*/
	HANDLE cFile = CreateFile(
		stringToLPCWSTR(filecps_path),
		GENERIC_WRITE,
		FILE_SHARE_WRITE,
		0,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_COMPRESSED,
		NULL);
	if (cFile == INVALID_HANDLE_VALUE) {
		std::cout << "File Open Error: " << filecps_path << endl;
		return 0;
	}
	
	string strBuf;
	unsigned long long ValidMsgSize = 0;
	DWORD dwReadNum;
	while (ReadFile(hFile, readBuf, 256, &dwReadNum, NULL)) {
		if (dwReadNum == 0) break;
		//读文件，每次读一个字节，并将对应的Huffman编码插入strBuf
		for (unsigned int i = 0; i < dwReadNum; ++i) {
			strBuf.append(HuffmanCodeTable[readBuf[i]]);
			ValidMsgSize += HuffmanCodeTable[readBuf[i]].length();
			//std::cout << "Huffman编码：" << HuffmanCodeTable[readBuf[i]].c_str() << endl;
		}
		//清理缓冲区
		memset(readBuf, 0, sizeof(readBuf));
	}
	CloseHandle(hFile);
	//cout << "ValidMsgSize有效信息位数:" << ValidMsgSize << endl;
	//strBuf不足补零
	//获取strBuf的最大长度maxlen
	unsigned long long maxlen = 8;
	while (strBuf.length() >= maxlen) {
		maxlen += 8;
	}
	int len = strBuf.length();
	for (int i = 0; i < maxlen - len; ++i) {
		strBuf.append("0");
	}

	//写文件头（配置信息所占字节数，有效压缩信息大小，配置信息）
	unsigned char Buf256[256] = { 0 }; //256位缓冲区
	unsigned char Buf8[8] = { 0 }; //8位缓冲区
	dwReadNum = 0;
	//计算配置信息的所占字节数 / 2 = ConfLen，即有效字节种类
	int ConfLen = 0;
	for (int i = 0; i < 256; ++i) {
		if (ByteWeight[i] != 0) {
			ConfLen++;
		}
	}
	//cout << "ConfLen有效字节种类：" << ConfLen << endl;
	//写入配置信息所占字节数 ConfLen < 256 即有效字节种类数
	unsigned char ConfLench = ConfLen;
	unsigned char* tmpch = new unsigned char[1];
	tmpch[0] = ConfLench;
	DWORD dwWriteNum;
	WriteFile(cFile, tmpch, 1, &dwWriteNum, NULL);
	//写入有效压缩信息的大小
	bitset<40> bit40(ValidMsgSize);
	string vaBuf = bit40.to_string(); //01串
	unsigned char* vaDecBuf = new unsigned char[5];
	int vaDecBufLen = 0;
	memset(vaDecBuf, 0, 5);
	for (size_t i = 0; i < vaBuf.length(); i += 8) {
		bitset<8> bit8(vaBuf.substr(i, 8));
		unsigned long dec = bit8.to_ulong();
		unsigned char tmpdec = dec;
		vaDecBuf[vaDecBufLen++] = tmpdec;
	}
	WriteFile(cFile, vaDecBuf, 5, &dwWriteNum, NULL);
	//写入配置信息（每种字节的权重）
	unsigned char* curByte = new unsigned char[2];
	memset(curByte, 0, 2);
	for (int i = 0; i < 256; ++i) {
		if (ByteWeight[i] != 0) {
			//将该字节写入
			curByte[0] = (unsigned char)i;
			//将该字节的权重写入
			curByte[1] = (unsigned char)ByteWeight[i];
			WriteFile(cFile, curByte, 2, &dwWriteNum, NULL);
		}
	}
	
	//取strBuf的子串，每次取8位，得到其十进制dec，将其写入writeBuf
	//string writeBuf;
	unsigned char* writeBuf = new unsigned char[ValidMsgSize+1];
	unsigned long writeBufLen = 0;
	memset(writeBuf, 0, sizeof(writeBuf));
	for (size_t i = 0; i < strBuf.length(); i += 8) {
		bitset<8> bit8(strBuf.substr(i, 8));
		unsigned long dec = bit8.to_ulong();
		unsigned char tmpdec = dec;
		writeBuf[writeBufLen++] = tmpdec;
	}
	//将writeBuf写入文件
	WriteFile(cFile, writeBuf, writeBufLen, &dwWriteNum, NULL);
	//cout << "writeBuf: " << writeBuf << endl
	//	<< "写出字节数 " << writeBuf.length() << endl;
	CloseHandle(cFile);
	return true;
}

//解压缩文件
bool UnCompressFile(string filecps_path) {
	//1.以只读+二进制模式打开压缩文件file_path.cps
	HANDLE cFile = CreateFile(
		stringToLPCWSTR(filecps_path),
		GENERIC_READ,
		FILE_SHARE_READ,
		0,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_READONLY,
		NULL);
	if (cFile == INVALID_HANDLE_VALUE) {
		std::cout << "打开文件(" << filecps_path << ")失败" << endl;
		return false;
	}
	//2.先读文件头（配置信息所占字节数，有效压缩信息大小，配置信息）
	unsigned char Buf8[1] = { 0 }; //8位缓冲区
	unsigned char Buf40[5] = { 0 }; //40位缓冲区
	
	//读取配置信息所占字节数
	DWORD dwReadNum, dwWriteNum;
	ReadFile(cFile, Buf8, 1, &dwReadNum, NULL);
	bitset<8> bit8(*Buf8);
	unsigned long ConfLen = bit8.to_ulong(); //配置信息的长度
	//cout << ConfLen << endl;
	//读取有效压缩信息的大小
	ReadFile(cFile, Buf40, 5, &dwReadNum, NULL);
	//cout << Buf40 << endl;
	
	unsigned long long ValidMsgSize = 0;
	bitset<8> bitF1;
	for (int i = 0; i < 5; ++i) {
		bitF1 = Buf40[i];
		ValidMsgSize = ValidMsgSize + (bitF1.to_ullong() << (8*(4-i)));
	}
	
	//cout << ValidMsgSize << endl;
	//读取配置信息
	memset(ByteWeight, 0, sizeof(ByteWeight));
	for (unsigned long i = 0; i < ConfLen; ++i) {
		//读取该字节
		ReadFile(cFile, Buf8, 1, &dwReadNum, NULL);
		if (dwReadNum == 0) break;
		unsigned char tmp = Buf8[0];
		//读取该字节的权重
		ReadFile(cFile, Buf8, 1, &dwReadNum, NULL);
		if (dwReadNum == 0) break;
		ByteWeight[tmp] += Buf8[0];
	}
	//重新构造Huffman树
	while(!PQ.empty())
		PQ.pop();
	InsertPQ();
	TreeNode reRoot;
	MakeHuffmanTree(reRoot);
	char Buf[256] = { 0 };
	GetHuffmanCode(reRoot, Buf, 0, HuffmanCodeTable); //获取Huffman编码
	

	//3.读压缩信息，解码到新文件
	HANDLE nFile = CreateFile(
		stringToLPCWSTR(file_ucps_path),
		GENERIC_WRITE,
		FILE_SHARE_WRITE,
		0,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (cFile == INVALID_HANDLE_VALUE) {
		std::cout << "File Open Error: " << file_ucps_path << endl;
		return 0;
	}
	
	unsigned char decodeBuf[1024];
	int decodeBufLen = 0;
	ReadFile(cFile, Buf8, 1, &dwReadNum, NULL);
	bit8 = *Buf8;
	string codeBufstr = bit8.to_string();
	//cout << codeBufstr << endl;
	while(BufPageNum*8+cPos < ValidMsgSize){
		TreeNode* Root = &reRoot;
		Decode(cFile, nFile, dwWriteNum, dwReadNum, Root, ValidMsgSize, codeBufstr, decodeBuf, decodeBufLen, cPos, dPos);
		//cout << codeBufstr << endl;
		//cout << BufPageNum << " " << cPos << endl;
		//cout << decodeBuf << endl;
	}
	//将缓冲区数据写入文件
	WriteFile(nFile, decodeBuf, decodeBufLen, &dwWriteNum, NULL);
	//清理缓冲区
	codeBufstr.clear();
	memset(decodeBuf, 0, sizeof(decodeBuf));
	CloseHandle(cFile);
	CloseHandle(nFile);
	return true;
}

void Decode(HANDLE& cFile, HANDLE& nFile,DWORD& dwWriteNum, DWORD& dwReadNum, TreeNode* reRoot, unsigned long long ValidMsgSize, string& codeBufstr, unsigned char decodeBuf[], int& decodeBufLen, int& cPos, int& dPos) {
	if (reRoot->Left == NULL && reRoot->Right == NULL) {
		//抵达叶子结点
		decodeBuf[decodeBufLen++] = reRoot->val;
		dPos++;
		return;
	}
	if (cPos == 8) {
		//编码缓冲区到底，但是没有找到对应解码，此时更新编码缓冲区，继续匹配
		codeBufstr.clear();
		unsigned char Buf8[1] = { 0 };
		ReadFile(cFile, Buf8, 1, &dwReadNum, NULL);
		BufPageNum++;
		if (dwReadNum == 0) return;
		bitset<8> Decodebit(*Buf8);
		codeBufstr = Decodebit.to_string();
		cPos = 0;
		
		Decode(cFile, nFile, dwWriteNum, dwReadNum, reRoot, ValidMsgSize, codeBufstr, decodeBuf, decodeBufLen, cPos, dPos);
		
		return;
	}
	if (dPos == 1024) {
		//解码缓冲区已满，写入文件
		WriteFile(nFile, decodeBuf, decodeBufLen, &dwWriteNum, NULL);
		memset(decodeBuf, 0, sizeof(decodeBuf));
		decodeBufLen = 0;
		dPos = 0;
	}
	if (codeBufstr[cPos] == '0') {
		cPos++;
		Decode(cFile, nFile, dwWriteNum, dwReadNum, reRoot->Left, ValidMsgSize, codeBufstr, decodeBuf, decodeBufLen, cPos, dPos);
	}
	else if (codeBufstr[cPos] == '1') {
		cPos++;
		Decode(cFile, nFile, dwWriteNum, dwReadNum, reRoot->Right, ValidMsgSize, codeBufstr, decodeBuf, decodeBufLen, cPos, dPos);
	}
}