#define _XOPEN_SOURCE
#include <iostream>
#include <fstream>
#include <atomic>
#include <thread>
#include <chrono>
#include <mutex>
extern "C"
{
    #include <unistd.h>
}

using namespace std;

struct { string word_list, output_file; int max_threads = 10; } config;
atomic<int> active(0); // active number of threads
atomic<long int> attempted(0); // number of hashes tried

class attempt
{
public:
  bool try_crypt(string hash, string word)
  {
    return (crypt(word.c_str(), (hash.substr(0,2)).c_str()) == hash ? true : false);
  }
};

class controller : public attempt
{
public:
  void file_write(string data, string file)
  {
    while(1)
    {
      if(hash_write.try_lock())
      {
        ofstream hash_output(file, ios::app);
        if(hash_output.is_open())
        {
          hash_output << data;
          hash_output.close();
          hash_write.unlock();    
          return;
        }
        else
        {
          cout << "\e[31m[" << time(0) << "] [-] Error Opening: '" << file << "' For writing\e[39m" << endl;
          exit(1);      
        }
      }
      this_thread::sleep_for(chrono::milliseconds(10));
    }
  }
  void crack_init(atomic<long int> &attempted, string hash, string wordlist, string ofile)
  {
    ifstream word_list_read(wordlist);
    string word;
    if(word_list_read.is_open())
    {
       while(getline(word_list_read, word))
       {
         ++attempted;
         if(try_crypt(hash, word))
         {
           if(!ofile.empty())
           {
             file_write(hash + ":" + word + "\n", ofile);
           }
           cout << "\e[32m[" << time(0) << "] [+] Cracked: '" << hash << "' Real Password: '" << word << "'\e[39m" << endl;
           return;
         }
       }
    }
    else
    {
      cout << "\e[31m[" << time(0) << "] [-] Error Opening: '" << wordlist << "' For Reading\e[39m" << endl;
      exit(1);
    }
  }
  void thread_create(atomic<int> &counter, atomic<long int> &attempted, string hash, string word_list, string ofile)
  {
    ++counter;
    crack_init(ref(attempted), hash, word_list, ofile);
    --counter;
  }
  void file_read(string file)
  {
    ifstream read_hashes(file);
    string hash;
    if(read_hashes.is_open())
    {
      while(getline(read_hashes, hash))
      {
        while(active >= config.max_threads)
        {
          this_thread::sleep_for(chrono::milliseconds(20));
        } //attempted
        thread(&controller::thread_create, this, ref(active), ref(attempted), hash, config.word_list, config.output_file).detach();
        this_thread::sleep_for(chrono::milliseconds(5));
      }
      while (active != 0)
      {
        this_thread::sleep_for(chrono::milliseconds(20));
      }
    }
    else
    {
      cout << "\e[31m[" << time(0) << "] [-] Error Opening: '" << file << "' For Reading\e[39m" << endl;
      exit(1);
    }
  } 
  controller(string list_or_hash, bool is_list)
  {
    if(is_list)
    {
      file_read(list_or_hash);
    }
    else
    {
      crack_init(ref(attempted), list_or_hash, config.word_list, config.output_file);
    }
    cout << "\e[33m[" << time(0) << "] [!] [INFO] Total Number of Hashes Tried: '" <<  attempted << "'\e[39m" << endl;
  }
private:
  mutex hash_write; 
};

int main(int argc, char **argv)
{
  cout << "\e[31m" << R"(#####  #####  ####     #   # ####    #   #   # #   # ##### ####  
 #  #  #     #         #   # #   #  # #  #   # #  #  #     #   # 
 #  #  ####  #          #### ####  #####  #### ###   ####  ####  
 ####  #     #             # #     #   #     # #  #  #     #     
#    # #####  ####         # #     #   #     # #   # ##### #)" << endl 
<< "\t\tDEScrypt CPU Collision Cracker V1.0\e[39m" << endl;
  cout << "For questions or bug fixes, either make a github commit or email, rek7@protonmail.com" << endl;
  if(argc >= 3)
  {
    string hashes, identifier;
    bool is_list = false;
    for(int i = 1; i < argc;i++)
    {
      if(sizeof(argv[i]) > 3)
      {
        identifier = string(argv[i]).substr(0, 3);
        if(identifier == "-l=")
        {
          is_list = true;
          hashes = string(argv[i]).substr(3, string(argv[i]).size());
        }
        else if(identifier == "-w=")
        {
          config.word_list = string(argv[i]).substr(3, string(argv[i]).size());
        }
        else if(identifier == "-t=")
        {
          config.max_threads = stoi(string(argv[i]).substr(3, string(argv[i]).size()));
        }
        else if(identifier == "-o=")
        {
          config.output_file = string(argv[i]).substr(3, string(argv[i]).size());
        }
        else if(identifier == "-i=")
        {
          hashes = string(argv[i]).substr(3, string(argv[i]).size());
        }
      }
    }
    cout << "\e[33m[" << time(0) << "] [!] [LOADED INFO] Wordlist: '" <<  config.word_list << "' Hash/Hash List: '" << hashes <<  "' Maximum Threads: '" << config.max_threads << "' Output File: '" << (!config.output_file.empty() ? config.output_file : "NONE") << "'\e[39m" << endl;
    cout << "\e[33m[" << time(0) << "] [!] [INFO] CPU Core(s) Detected: '" <<  thread::hardware_concurrency() << "'\e[39m" << endl;
    controller(hashes, is_list);
  }
  else
  {
    cout << "General Commands:" << endl;
    cout << "\tCommand\t\tDescription" << endl;
    cout << "\t-------\t\t-----------" << endl;
    cout << "\t-l=\t\tFile Location of HashList" << endl;
    cout << "\t-i=\t\tIndividual Hash" << endl;
    cout << "\t-w=\t\tFile Location of Wordlist" << endl;
    cout << "\t-o=\t\tFile Location of Output File" << endl;
    cout << "\t-t=\t\tNumber of Threads(Default is 10)" << endl;
    cout << "Example Usage: " <<  argv[0] << " -l=hashes.txt -w=dictonary.txt -t=20 -o=cracked.txt" << endl;
  }
  return 0;
}
