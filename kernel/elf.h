#include <stdint.h>
#define ELF_MAGIC 0x464C457FU
// typedef struct {
// 	unsigned char	e_ident[EI_NIDENT];	/* File identification. */
// 	Elf64_Half	e_type;		/* File type. */
// 	Elf64_Half	e_machine;	/* Machine architecture. */
// 	Elf64_Word	e_version;	/* ELF format version. */
// 	Elf64_Addr	e_entry;	/* Entry point. */
// 	Elf64_Off	e_phoff;	/* Program header file offset. */
// 	Elf64_Off	e_shoff;	/* Section header file offset. */
// 	Elf64_Word	e_flags;	/* Architecture-specific flags. */
// 	Elf64_Half	e_ehsize;	/* Size of ELF header in bytes. */
// 	Elf64_Half	e_phentsize;	/* Size of program header entry. */
// 	Elf64_Half	e_phnum;	/* Number of program header entries. */
// 	Elf64_Half	e_shentsize;	/* Size of section header entry. */
// 	Elf64_Half	e_shnum;	/* Number of section header entries. */
// 	Elf64_Half	e_shstrndx;	/* Section name strings section. */
// } Elf64_Ehdr;


struct elf_header {
    uint32_t magic;
    uint8_t e_ident_rest[12];
    uint16_t type;
    uint16_t machine;
    uint32_t version;
    uint64_t entry;
    uint64_t ph_off;
    uint64_t sh_off;
    uint32_t flags;
    uint16_t eh_size;
    uint16_t ph_entry_size; 
    uint16_t ph_num;
    uint16_t sh_entry_size;
    uint16_t sh_num;
    uint16_t sh_str_ndx;
};

// // File header
// struct elfhdr {
//   uint magic;  // must equal ELF_MAGIC
//   uchar elf[12];
//   ushort type;
//   ushort machine;
//   uint version;
//   uint entry;
//   uint phoff;
//   uint shoff;
//   uint flags;
//   ushort ehsize;
//   ushort phentsize;
//   ushort phnum;
//   ushort shentsize;
//   ushort shnum;
//   ushort shstrndx;
// }

// typedef struct {
// 	Elf64_Word	p_type;		/* Entry type. */
// 	Elf64_Word	p_flags;	/* Access permission flags. */
// 	Elf64_Off	p_offset;	/* File offset of contents. */
// 	Elf64_Addr	p_vaddr;	/* Virtual address in memory image. */
// 	Elf64_Addr	p_paddr;	/* Physical address (not used). */
// 	Elf64_Xword	p_filesz;	/* Size of contents in file. */
// 	Elf64_Xword	p_memsz;	/* Size of contents in memory. */
// 	Elf64_Xword	p_align;	/* Alignment in memory and file. */
// } Elf64_Phdr;
struct prog_header {
    uint32_t type;
    uint32_t flags;
    uint64_t offset;
    uint64_t vaddr;
    uint64_t paddr;
    uint64_t file_sz;
    uint64_t mem_sz;
    uint64_t align;
};

// Program section header
// struct proghdr {
//   uint type;
//   uint off;
//   uint vaddr;
//   uint paddr;
//   uint filesz;
//   uint memsz;
//   uint flags;
//   uint align;
// };

// Values for Proghdr type
#define ELF_PROG_LOAD           1 // load this to mem

// Flag bits for Proghdr flags
#define ELF_PROG_FLAG_EXEC      1 
#define ELF_PROG_FLAG_WRITE     2
#define ELF_PROG_FLAG_READ      4
