/* 
 * boot.img操作
 * -u in_file。解压boot.img
 * -r out_file。重新打包
 * wittered by boot
 * QQ:735718299
 *
 */
// #define debug

#include <fstream>
#include <iostream>
#include <cstring>
#include <cstdlib>

using namespace std;

void error();
void un_pack(char *file);
void re_pack(char *file);

struct boot_img_hdr
{
	char magic[8];
	
	unsigned kernel_size;
	char kernel_addr[4];
	
	unsigned ramdisk_size;
	char ramdisk_addr[4];

	unsigned second_size;
	char second_addr[4];

	char tags_addr[4];
	unsigned page_size;
	char unused[2];

	char name[16];
       	char cmdline[512];

	unsigned id[8];
} boot;



int main(int argc, char *argv[]) {

#ifdef debug
  printf("argc=%d\nargv=%s", argc, argv[1]);
#endif
  if (argc == 3) {
    if (argv[1][0] == '-' && argv[1][1] == 'u') {
      char *file_name = &argv[2][0];
      un_pack(file_name);
    } else if (argv[1][0] == '-' && argv[1][1] == 'r') {
      char *file_name = &argv[2][0];
      re_pack(file_name);
    } else
      error();
  } else
    error();
}
void error() {
  cout << "Usage:" << endl << endl;
  cout << "    -u file     unpack an img file" << endl;
  cout << "    -r file     repack to an img file" << endl;
  cout << endl << "Made by Boot<booting.0122@gmail.com>" << endl;
  exit(0);
}

void un_pack(char *file) {
  fstream in_file, out_kernel, out_ramdisk, out_second, config;
  in_file.open(file, ios::binary | ios::in);
  if (!in_file) {

    cerr << "\033[1;31mFatal: \033[0mCould not open " << file <<
      ",is this file exist?" << endl;
    exit(0);
  }
  out_kernel.open("kernel", ios::binary | ios::out);
  out_ramdisk.open("ramdisk.gz", ios::binary | ios::out);
  out_second.open("second_stage", ios::binary | ios::out);
  config.open("config", ios::binary | ios::out);
#ifdef debug
  cout << endl << endl << file << endl << *file << endl;
#endif

  long int kernel_size, ramdisk_size, sec_stage_size, page;
  char k_s[4],r_s[4],s_s[4],pg[4]; /* 创建数值储存*/

  in_file.seekg(0,ios::beg);
  in_file.read(boot.magic,8);
  in_file.read(k_s,4);
  in_file.read(boot.kernel_addr,4);
  in_file.read(r_s,4);
  in_file.read(boot.ramdisk_addr,4);
  in_file.read(s_s,4);
  in_file.read(boot.second_addr,4);
  in_file.read(boot.tags_addr,4);
  in_file.read(pg,4);
  in_file.read(boot.unused,8);
  in_file.read(boot.name,16);
  in_file.read(boot.cmdline,512);

  memcpy(&boot.kernel_size, k_s, 4); /* 小端读取 */
  memcpy(&boot.ramdisk_size, r_s, 4);
  memcpy(&boot.second_size, s_s, 4);
  memcpy(&boot.page_size, pg, 4);

  /* 写入必要配置文件，重新打包需要 */
  config.seekp(0, ios::beg);
  config.write(boot.magic,8);
  config.write(boot.kernel_addr,4);
  config.write(boot.ramdisk_addr,4);
  config.write(boot.second_addr,4);
  config.write(boot.tags_addr,4);
  config.write(pg,4);
  config.write(boot.unused,8);
  config.write(boot.name,16);
  config.write(boot.cmdline,512);

  cout << endl << "\033[7mkernel_size:\033[0m " << boot.kernel_size << '\n' <<
    "\033[7mramdisk_size:\033[0m " << boot.ramdisk_size << '\n' <<
    "\033[7msecond stage size\033[0m  " << boot.second_size << '\n' <<
    "\033[7mpage size:\033[0m  " << boot.page_size << "\nCmdline: " << boot.cmdline << endl;

  in_file.seekg(2048, ios::beg);
  /* 开始读取kernel */
  char *kernel_buff = new char[boot.kernel_size];
  out_kernel.seekp(0, ios::beg);
  in_file.read(kernel_buff, boot.kernel_size);
  out_kernel.write(kernel_buff, boot.kernel_size);
  delete[]kernel_buff;
  /* 开始读取RamDisk */
  int ramdisk_addr = (((boot.kernel_size + boot.page_size) / (boot.page_size * 2)) + 1) * (boot.page_size * 2);
  in_file.seekg(ramdisk_addr, ios::beg);
  char *ramdisk_buff = new char[boot.ramdisk_size];
  out_ramdisk.seekp(0, ios::beg);
  in_file.read(ramdisk_buff, boot.ramdisk_size);
  out_ramdisk.write(ramdisk_buff, boot.ramdisk_size);
  delete[]ramdisk_buff;
  /*开始读取second_stage*/
  int second_addr = (((ramdisk_addr + boot.ramdisk_size) / (boot.page_size * 2)) + 1) * (boot.page_size * 2);
  in_file.seekg(second_addr, ios::beg);
  char *second_buff = new char[boot.second_size];
  out_ramdisk.seekp(0, ios::beg);
  in_file.read(second_buff, boot.second_size);
  out_second.write(second_buff, boot.second_size);
  delete[]second_buff;

  cout << "\nUnpack  finished!" << endl;
  in_file.close();
  out_kernel.close();
  out_ramdisk.close();
  out_second.close();
  config.close();
}

void re_pack(char *file) {
  fstream in_kernel, in_ramdisk, out_file, in_second,config;
  char k_s[4], r_s[4],s_s[4],pg[4];
  char magic[] = { 0x41, 0x4E, 0x44, 0x52, 0x4F, 0x49, 0x44, 0x21 };
  
  cout << "Finding Config file…" << flush;
  config.open("config",ios::in | ios::binary);
  if (!config) {
    cerr <<
      "   [Failed]\n\nFatal: Could not find config!\nYou can not create file without config\nOr you may not extract by this tool!\n"
      << endl;
    exit(0);
  } else {
    cout << "   [OK]" << endl;
  }

  //读取配置文件，建立结构体
  config.seekg(0,ios::beg);
  config.read(boot.magic,8);
  config.read(boot.kernel_addr,4);
  config.read(boot.ramdisk_addr,4);
  config.read(boot.second_addr,4);
  config.read(boot.tags_addr,4);
  config.read(pg,4);
  config.read(boot.unused,8);
  config.read(boot.name,16);
  config.read(boot.cmdline,512);
  //读取完毕，结构体已建立 

  

  cout << "Finding kernel…" << flush;
  in_kernel.open("kernel", ios::in | ios::binary);
  if (!in_kernel) {
    cerr <<
      "   [Failed]\n\nFatal: Could not find kernel!\nMake sure you put kernel in this folder"
      << endl;
    exit(0);
  } else {
    cout << "   [OK]" << endl;
    in_kernel.seekg(0, ios::end);
    boot.kernel_size = in_kernel.tellg();
    cout << "Kernel size:" << boot.kernel_size << endl;
  }
  

  cout << "Finding ramdisk.gz…" << flush;
  in_ramdisk.open("ramdisk.gz", ios::in | ios::binary);
  if (!in_ramdisk) {
    cerr <<
      "   [Failed]\n\nFatal: Could not find ramdisk.gz!\nMake sure you put ramdisk.gz in this folder"
      << endl;
    exit(0);
  } else {
    cout << "   [OK]" << endl;
    in_ramdisk.seekg(0, ios::end);
    boot.ramdisk_size = in_ramdisk.tellg();
    cout << "Ramdisk size:" << boot.ramdisk_size << endl;
  }
 

  cout << "Finding second stage…" << flush;
  in_second.open("second_stage", ios::in | ios::binary);
  if (!in_second) {
    cerr <<
      "   [Skip]\n\nWarning: Could not find second stage!\n  Skip!"<< endl;
    boot.second_size = 0;
  } else {
    cout << "   [OK]" << endl;
    in_second.seekg(0, ios::end);
    boot.second_size = in_second.tellg();
  }
  cout<< "Second stage size:"<<boot.second_size<<endl;
  
  memcpy(k_s,&boot.kernel_size,  4);
  memcpy(r_s,&boot.ramdisk_size, 4);
  memcpy(s_s,&boot.second_size, 4);
  memcpy(&boot.page_size, pg, 4);
  //创建文件头
  out_file.open(file,ios::out | ios::binary);
  out_file.seekg(0,ios::beg);
  out_file.write(boot.magic,8);
  out_file.write(k_s,4);
  out_file.write(boot.kernel_addr,4);
  out_file.write(r_s,4);
  out_file.write(boot.ramdisk_addr,4);
  out_file.write(s_s,4);
  out_file.write(boot.second_addr,4);
  out_file.write(boot.tags_addr,4);
  out_file.write(pg,4);
  out_file.write(boot.unused,8);
  out_file.write(boot.name,16);
  out_file.write(boot.cmdline,512);

  //开始写入kernel
  char *kernel_buff = new char[boot.kernel_size];
  in_kernel.seekg(0,ios::beg);
  in_kernel.read(kernel_buff,boot.kernel_size);
  out_file.seekp(boot.page_size,ios::beg);
  out_file.write(kernel_buff,boot.kernel_size);
  delete []kernel_buff;

  int ramdisk_addr = (((boot.kernel_size + boot.page_size)/ (boot.page_size * 2)) + 1) * (boot.page_size * 2);

  //开始写入ramdisk
  char *ramdisk_buff = new char[boot.ramdisk_size];
  in_ramdisk.seekg(0,ios::beg);
  in_ramdisk.read(ramdisk_buff,boot.ramdisk_size);
  out_file.seekp(ramdisk_addr,ios::beg);
  out_file.write(ramdisk_buff,boot.ramdisk_size);
  delete []ramdisk_buff;

  int second_addr = ((( boot.ramdisk_size / (boot.page_size * 2)) + 1) * (boot.page_size * 2)) + ramdisk_addr;
  //开始写入second stage
  char *second_buff = new char[boot.second_size];
  in_second.seekg(0,ios::beg);
  in_second.read(second_buff,boot.second_size);
  out_file.seekp(second_addr,ios::beg);
  out_file.write(second_buff,boot.second_size);
  delete []second_buff;
  
  in_kernel.close();
  in_ramdisk.close();
  in_second.close();
  out_file.close();

  cout<<"\nFinished!";

}