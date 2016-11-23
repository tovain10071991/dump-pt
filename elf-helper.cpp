#include "elf-helper.h"

#include <gelf.h>

#include <map>
#include <string>

#include <err.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>

using namespace std;

static map<string, Elf*> elfs;
static map<string, int> fds;

class ELFInited {
public:
  ELFInited()
  {
    if(elf_version(EV_CURRENT)==EV_NONE)
      errx(elf_errno(), "elf_version: %s\n", elf_errmsg(elf_errno()));
  }
  ~ELFInited()
  {
    for(auto elf_iter = elfs.begin(); elf_iter != elfs.end(); ++elf_iter)
      elf_end(elf_iter->second);
  }
};
static ELFInited inited;

static Elf* get_elf(std::string binary) {
  if(elfs.find(binary) == elfs.end()) {
    int fd = open(binary.c_str(), O_RDONLY);
    assert(fd!=-1);
    if(!(elfs[binary]=elf_begin(fd, ELF_C_READ, NULL)))
      errx(elf_errno(), "%s elf_begin: %s\n", binary.c_str(), elf_errmsg(elf_errno()));
    fds[binary] = fd;
  }
  return elfs[binary];
}

static int get_fd(std::string binary) {
  if(fds.find(binary) == fds.end())
    get_elf(binary);
  assert(fds.find(binary) != fds.end());
  return fds[binary];
}

static string get_sec_name(string binary, unsigned sh_name_idx) {
  size_t shdr_str_idx;
  Elf* elf = get_elf(binary);
  assert(elf_getshdrstrndx(elf, &shdr_str_idx)!=-1);
  return elf_strptr(elf, shdr_str_idx, sh_name_idx);
}

static Elf_Scn* get_sec(string binary, string sec_name) {
  Elf* elf = get_elf(binary);
  Elf_Scn* scn =NULL;
  GElf_Shdr shdr;
	for(scn=elf_nextscn(elf, scn); scn; scn=elf_nextscn(elf, scn))
	{
    assert(gelf_getshdr(scn, &shdr)!=NULL);
		if(!get_sec_name(binary, shdr.sh_name).compare(sec_name))
		{
      return scn;
		}
	}
  errx(-1, "%s section not found", sec_name.c_str());  
}

static GElf_Shdr get_sec_header(string binary, string sec_name) {
  Elf_Scn* scn = get_sec(binary, sec_name);
  GElf_Shdr shdr;
  assert(gelf_getshdr(scn, &shdr)!=NULL);
  return shdr;
}

uint64_t get_entry(string binary) {
  GElf_Ehdr ehdr;
  Elf* elf = get_elf(binary);
  assert(gelf_getehdr(elf, &ehdr)!=NULL);
  return ehdr.e_entry;
}

size_t get_text_sec_start_off(string binary) {
  GElf_Shdr shdr = get_sec_header(binary, ".text");
  return shdr.sh_offset;
}

size_t get_text_sec_end_off(string binary) {
  GElf_Shdr shdr = get_sec_header(binary, ".text");
  return shdr.sh_offset + shdr.sh_size;
}

size_t get_text_sec_start_addr(string binary) {
  GElf_Shdr shdr = get_sec_header(binary, ".text");
  return shdr.sh_addr;
}

size_t get_content(string binary, size_t offset, void* buf, size_t size) {
  int fd = get_fd(binary);
  assert(lseek(fd, offset, SEEK_SET)!=-1);
  size_t ret_size = read(fd, buf, size);
  assert(ret_size != (size_t)-1);
  return ret_size;
}