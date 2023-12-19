// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2020 Google, Inc.

/// @file
///
/// This contains the definitions of the ELF utilities for the dwarf reader.

#include "abg-elf-helpers.h"

#include <elf.h>

#include "abg-tools-utils.h"

namespace abigail
{

namespace elf_helpers
{

/// Convert an elf symbol type (given by the ELF{32,64}_ST_TYPE
/// macros) into an elf_symbol::type value.
///
/// Note that this function aborts when given an unexpected value.
///
/// @param the symbol type value to convert.
///
/// @return the converted value.
elf_symbol::type
stt_to_elf_symbol_type(unsigned char stt)
{
  switch (stt)
    {
    case STT_NOTYPE:
      return elf_symbol::NOTYPE_TYPE;
    case STT_OBJECT:
      return elf_symbol::OBJECT_TYPE;
    case STT_FUNC:
      return elf_symbol::FUNC_TYPE;
    case STT_SECTION:
      return elf_symbol::SECTION_TYPE;
    case STT_FILE:
      return elf_symbol::FILE_TYPE;
    case STT_COMMON:
      return elf_symbol::COMMON_TYPE;
    case STT_TLS:
      return elf_symbol::TLS_TYPE;
    case STT_GNU_IFUNC:
      return elf_symbol::GNU_IFUNC_TYPE;
    default:
      // An unknown value that probably ought to be supported?  Let's
      // abort right here rather than yielding garbage.
      ABG_ASSERT_NOT_REACHED;
    }
}

/// Convert an elf symbol binding (given by the ELF{32,64}_ST_BIND
/// macros) into an elf_symbol::binding value.
///
/// Note that this function aborts when given an unexpected value.
///
/// @param the symbol binding value to convert.
///
/// @return the converted value.
elf_symbol::binding
stb_to_elf_symbol_binding(unsigned char stb)
{
  switch (stb)
    {
    case STB_LOCAL:
      return elf_symbol::LOCAL_BINDING;
    case STB_GLOBAL:
      return elf_symbol::GLOBAL_BINDING;
    case STB_WEAK:
      return elf_symbol::WEAK_BINDING;
    case STB_GNU_UNIQUE:
      return elf_symbol::GNU_UNIQUE_BINDING;
    default:
      ABG_ASSERT_NOT_REACHED;
    }
}

/// Convert an ELF symbol visiblity given by the symbols ->st_other
/// data member as returned by the GELF_ST_VISIBILITY macro into a
/// elf_symbol::visiblity value.
///
/// @param stv the value of the ->st_other data member of the ELF
/// symbol.
///
/// @return the converted elf_symbol::visiblity value.
elf_symbol::visibility
stv_to_elf_symbol_visibility(unsigned char stv)
{
  switch (stv)
    {
    case STV_DEFAULT:
      return elf_symbol::DEFAULT_VISIBILITY;
    case STV_INTERNAL:
      return elf_symbol::INTERNAL_VISIBILITY;
    case STV_HIDDEN:
      return elf_symbol::HIDDEN_VISIBILITY;
    case STV_PROTECTED:
      return elf_symbol::PROTECTED_VISIBILITY;
    default:
      ABG_ASSERT_NOT_REACHED;
    }
}

/// Convert the value of the e_machine field of GElf_Ehdr into a
/// string.  This is to get a string representing the architecture of
/// the elf file at hand.
///
/// @param e_machine the value of GElf_Ehdr::e_machine.
///
/// @return the string representation of GElf_Ehdr::e_machine.
std::string
e_machine_to_string(GElf_Half e_machine)
{
  switch (e_machine)
    {
    case EM_NONE:
      return "elf-no-arch";
    case EM_M32:
      return "elf-att-we-32100";
    case EM_SPARC:
      return "elf-sun-sparc";
    case EM_386:
      return "elf-intel-80386";
    case EM_68K:
      return "elf-motorola-68k";
    case EM_88K:
      return "elf-motorola-88k";
    case EM_860:
      return "elf-intel-80860";
    case EM_MIPS:
      return "elf-mips-r3000-be";
    case EM_S370:
      return "elf-ibm-s370";
    case EM_MIPS_RS3_LE:
      return "elf-mips-r3000-le";
    case EM_PARISC:
      return "elf-hp-parisc";
    case EM_VPP500:
      return "elf-fujitsu-vpp500";
    case EM_SPARC32PLUS:
      return "elf-sun-sparc-v8plus";
    case EM_960:
      return "elf-intel-80960";
    case EM_PPC:
      return "elf-powerpc";
    case EM_PPC64:
      return "elf-powerpc-64";
    case EM_S390:
      return "elf-ibm-s390";
    case EM_V800:
      return "elf-nec-v800";
    case EM_FR20:
      return "elf-fujitsu-fr20";
    case EM_RH32:
      return "elf-trw-rh32";
    case EM_RCE:
      return "elf-motorola-rce";
    case EM_ARM:
      return "elf-arm";
    case EM_FAKE_ALPHA:
      return "elf-digital-alpha";
    case EM_SH:
      return "elf-hitachi-sh";
    case EM_SPARCV9:
      return "elf-sun-sparc-v9-64";
    case EM_TRICORE:
      return "elf-siemens-tricore";
    case EM_ARC:
      return "elf-argonaut-risc-core";
    case EM_H8_300:
      return "elf-hitachi-h8-300";
    case EM_H8_300H:
      return "elf-hitachi-h8-300h";
    case EM_H8S:
      return "elf-hitachi-h8s";
    case EM_H8_500:
      return "elf-hitachi-h8-500";
    case EM_IA_64:
      return "elf-intel-ia-64";
    case EM_MIPS_X:
      return "elf-stanford-mips-x";
    case EM_COLDFIRE:
      return "elf-motorola-coldfire";
    case EM_68HC12:
      return "elf-motorola-68hc12";
    case EM_MMA:
      return "elf-fujitsu-mma";
    case EM_PCP:
      return "elf-siemens-pcp";
    case EM_NCPU:
      return "elf-sony-ncpu";
    case EM_NDR1:
      return "elf-denso-ndr1";
    case EM_STARCORE:
      return "elf-motorola-starcore";
    case EM_ME16:
      return "elf-toyota-me16";
    case EM_ST100:
      return "elf-stm-st100";
    case EM_TINYJ:
      return "elf-alc-tinyj";
    case EM_X86_64:
      return "elf-amd-x86_64";
    case EM_PDSP:
      return "elf-sony-pdsp";
    case EM_FX66:
      return "elf-siemens-fx66";
    case EM_ST9PLUS:
      return "elf-stm-st9+";
    case EM_ST7:
      return "elf-stm-st7";
    case EM_68HC16:
      return "elf-motorola-68hc16";
    case EM_68HC11:
      return "elf-motorola-68hc11";
    case EM_68HC08:
      return "elf-motorola-68hc08";
    case EM_68HC05:
      return "elf-motorola-68hc05";
    case EM_SVX:
      return "elf-sg-svx";
    case EM_ST19:
      return "elf-stm-st19";
    case EM_VAX:
      return "elf-digital-vax";
    case EM_CRIS:
      return "elf-axis-cris";
    case EM_JAVELIN:
      return "elf-infineon-javelin";
    case EM_FIREPATH:
      return "elf-firepath";
    case EM_ZSP:
      return "elf-lsi-zsp";
    case EM_MMIX:
      return "elf-don-knuth-mmix";
    case EM_HUANY:
      return "elf-harvard-huany";
    case EM_PRISM:
      return "elf-sitera-prism";
    case EM_AVR:
      return "elf-atmel-avr";
    case EM_FR30:
      return "elf-fujistu-fr30";
    case EM_D10V:
      return "elf-mitsubishi-d10v";
    case EM_D30V:
      return "elf-mitsubishi-d30v";
    case EM_V850:
      return "elf-nec-v850";
    case EM_M32R:
      return "elf-mitsubishi-m32r";
    case EM_MN10300:
      return "elf-matsushita-mn10300";
    case EM_MN10200:
      return "elf-matsushita-mn10200";
    case EM_PJ:
      return "elf-picojava";
    case EM_OPENRISC:
      return "elf-openrisc-32";
    case EM_ARC_A5:
      return "elf-arc-a5";
    case EM_XTENSA:
      return "elf-tensilica-xtensa";

#ifdef HAVE_EM_AARCH64_MACRO
    case EM_AARCH64:
      return "elf-arm-aarch64";
#endif

#ifdef HAVE_EM_TILEPRO_MACRO
    case EM_TILEPRO:
      return "elf-tilera-tilepro";
#endif

#ifdef HAVE_EM_TILEGX_MACRO
    case EM_TILEGX:
      return "elf-tilera-tilegx";
#endif

    case EM_NUM:
      return "elf-last-arch-number";
    case EM_ALPHA:
      return "elf-non-official-alpha";
    default:
      {
	std::ostringstream o;
	o << "elf-unknown-arch-value-" << e_machine;
	return o.str();
      }
    }
}

/// Find and return a section by its name and its type.
///
/// @param elf_handle the elf handle to use.
///
/// @param name the name of the section.
///
/// @param section_type the type of the section.  This is the
/// Elf32_Shdr::sh_type (or Elf64_Shdr::sh_type) data member.
/// Examples of values of this parameter are SHT_PROGBITS or SHT_NOBITS.
///
/// @return the section found, nor nil if none was found.
Elf_Scn*
find_section(Elf* elf_handle, const std::string& name, Elf64_Word section_type)
{
  size_t section_header_string_index = 0;
  if (elf_getshdrstrndx (elf_handle, &section_header_string_index) < 0)
    return 0;

  Elf_Scn* section = 0;
  GElf_Shdr header_mem, *header;
  while ((section = elf_nextscn(elf_handle, section)) != 0)
    {
      header = gelf_getshdr(section, &header_mem);
      if (header == NULL || header->sh_type != section_type)
      continue;

      const char* section_name =
	elf_strptr(elf_handle, section_header_string_index, header->sh_name);
      if (section_name && name == section_name)
	return section;
    }

  return 0;
}

/// Find the symbol table.
///
/// If we are looking at a relocatable or executable file, this
/// function will return the .symtab symbol table (of type
/// SHT_SYMTAB).  But if we are looking at a DSO it returns the
/// .dynsym symbol table (of type SHT_DYNSYM).
///
/// @param elf_handle the elf handle to consider.
///
/// @param symtab the symbol table found.
///
/// @return the symbol table section
Elf_Scn*
find_symbol_table_section(Elf* elf_handle)
{
  Elf_Scn* section = 0, *dynsym = 0, *sym_tab = 0;
  while ((section = elf_nextscn(elf_handle, section)) != 0)
    {
      GElf_Shdr header_mem, *header;
      header = gelf_getshdr(section, &header_mem);
      if (header->sh_type == SHT_DYNSYM)
	dynsym = section;
      else if (header->sh_type == SHT_SYMTAB)
	sym_tab = section;
    }

  if (dynsym || sym_tab)
    {
      GElf_Ehdr eh_mem;
      GElf_Ehdr* elf_header = gelf_getehdr(elf_handle, &eh_mem);
      if (elf_header->e_type == ET_REL
	  || elf_header->e_type == ET_EXEC)
	return sym_tab ? sym_tab : dynsym;
      else
	return dynsym ? dynsym : sym_tab;
    }
  return NULL;
}

/// Find the index (in the section headers table) of the symbol table
/// section.
///
/// If we are looking at a relocatable or executable file, this
/// function will return the index for the .symtab symbol table (of
/// type SHT_SYMTAB).  But if we are looking at a DSO it returns the
/// index for the .dynsym symbol table (of type SHT_DYNSYM).
///
/// @param elf_handle the elf handle to use.
///
/// @param symtab_index the index of the symbol_table, that was found.
///
/// @return true iff the symbol table section index was found.
bool
find_symbol_table_section_index(Elf* elf_handle, size_t& symtab_index)
{
  Elf_Scn* section = find_symbol_table_section(elf_handle);

  if (!section)
    return false;

  symtab_index = elf_ndxscn(section);
  return true;
}

/// Get the offset offset of the hash table section.
///
/// @param elf_handle the elf handle to use.
///
/// @param ht_section_offset this is set to the resulting offset
/// of the hash table section.  This is set iff the function returns true.
///
/// @param symtab_section_offset the offset of the section of the
/// symbol table the hash table refers to.
hash_table_kind
find_hash_table_section_index(Elf*	elf_handle,
			      size_t&	ht_section_index,
			      size_t&	symtab_section_index)
{
  if (!elf_handle)
    return NO_HASH_TABLE_KIND;

  GElf_Shdr header_mem, *section_header;
  bool found_sysv_ht = false, found_gnu_ht = false;
  for (Elf_Scn* section = elf_nextscn(elf_handle, 0);
       section != 0;
       section = elf_nextscn(elf_handle, section))
    {
      section_header= gelf_getshdr(section, &header_mem);
      if (section_header->sh_type != SHT_HASH
	  && section_header->sh_type != SHT_GNU_HASH)
	continue;

      ht_section_index = elf_ndxscn(section);
      symtab_section_index = section_header->sh_link;

      if (section_header->sh_type == SHT_HASH)
	found_sysv_ht = true;
      else if (section_header->sh_type == SHT_GNU_HASH)
	found_gnu_ht = true;
    }

  if (found_gnu_ht)
    return GNU_HASH_TABLE_KIND;
  else if (found_sysv_ht)
    return SYSV_HASH_TABLE_KIND;
  else
    return NO_HASH_TABLE_KIND;
}

/// Find and return the .text section.
///
/// @param elf_handle the elf handle to use.
///
/// @return the .text section found.
Elf_Scn*
find_text_section(Elf* elf_handle)
{return find_section(elf_handle, ".text", SHT_PROGBITS);}

/// Find and return the .bss section.
///
/// @param elf_handle.
///
/// @return the .bss section found.
Elf_Scn*
find_bss_section(Elf* elf_handle)
{return find_section(elf_handle, ".bss", SHT_NOBITS);}

/// Find and return the .rodata section.
///
/// @param elf_handle.
///
/// @return the .rodata section found.
Elf_Scn*
find_rodata_section(Elf* elf_handle)
{return find_section(elf_handle, ".rodata", SHT_PROGBITS);}

/// Find and return the .data section.
///
/// @param elf_handle the elf handle to use.
///
/// @return the .data section found.
Elf_Scn*
find_data_section(Elf* elf_handle)
{return find_section(elf_handle, ".data", SHT_PROGBITS);}

/// Find and return the .data1 section.
///
/// @param elf_handle the elf handle to use.
///
/// @return the .data1 section found.
Elf_Scn*
find_data1_section(Elf* elf_handle)
{return find_section(elf_handle, ".data1", SHT_PROGBITS);}

/// Return the "Official Procedure descriptors section."  This
/// section is named .opd, and is usually present only on PPC64
/// ELFv1 binaries.
///
/// @param elf_handle the elf handle to consider.
///
/// @return the .opd section, if found.  Return nil otherwise.
Elf_Scn*
find_opd_section(Elf* elf_handle)
{return find_section(elf_handle, ".opd", SHT_PROGBITS);}

/// Return the SHT_GNU_versym, SHT_GNU_verdef and SHT_GNU_verneed
/// sections that are involved in symbol versionning.
///
/// @param elf_handle the elf handle to use.
///
/// @param versym_section the SHT_GNU_versym section found.  If the
/// section wasn't found, this is set to nil.
///
/// @param verdef_section the SHT_GNU_verdef section found.  If the
/// section wasn't found, this is set to nil.
///
/// @param verneed_section the SHT_GNU_verneed section found.  If the
/// section wasn't found, this is set to nil.
///
/// @return true iff at least one of the sections where found.
bool
get_symbol_versionning_sections(Elf*		elf_handle,
				Elf_Scn*&	versym_section,
				Elf_Scn*&	verdef_section,
				Elf_Scn*&	verneed_section)
{
  Elf_Scn* section = NULL;
  GElf_Shdr mem;
  Elf_Scn* versym = NULL, *verdef = NULL, *verneed = NULL;

  while ((section = elf_nextscn(elf_handle, section)) != NULL)
    {
      GElf_Shdr* h = gelf_getshdr(section, &mem);
      if (h->sh_type == SHT_GNU_versym)
	versym = section;
      else if (h->sh_type == SHT_GNU_verdef)
	verdef = section;
      else if (h->sh_type == SHT_GNU_verneed)
	verneed = section;
    }

  if (versym || verdef || verneed)
    {
      // At least one the versionning sections was found.  Return it.
      versym_section = versym;
      verdef_section = verdef;
      verneed_section = verneed;
      return true;
    }

  return false;
}

/// Return the __ksymtab section of a linux kernel ELF file (either
/// a vmlinux binary or a kernel module).
///
/// @param elf_handle the elf handle to consider.
///
/// @return the __ksymtab section if found, nil otherwise.
Elf_Scn*
find_ksymtab_section(Elf* elf_handle)
{return find_section(elf_handle, "__ksymtab", SHT_PROGBITS);}

/// Return the __ksymtab_gpl section of a linux kernel ELF file (either
/// a vmlinux binary or a kernel module).
///
/// @param elf_handle the elf handle to consider.
///
/// @return the __ksymtab section if found, nil otherwise.
Elf_Scn*
find_ksymtab_gpl_section(Elf* elf_handle)
{return find_section(elf_handle, "__ksymtab_gpl", SHT_PROGBITS);}

/// Find the __ksymtab_strings section of a Linux kernel binary.
///
/// @param elf_handle the elf handle to use.
///
/// @return the find_ksymtab_strings_section of the linux kernel
/// binary denoted by @p elf_handle, or nil if such a section could
/// not be found.
Elf_Scn*
find_ksymtab_strings_section(Elf *elf_handle)
{
  if (is_linux_kernel(elf_handle))
    return find_section(elf_handle, "__ksymtab_strings", SHT_PROGBITS);
  return 0;
}

/// Return the .rel{a,} section corresponding to a given section.
///
/// @param elf_handle the elf handle to consider.
///
/// @param target_section the section to search the relocation section for
///
/// @return the .rel{a,} section if found, null otherwise.
Elf_Scn*
find_relocation_section(Elf* elf_handle, Elf_Scn* target_section)
{
  if (target_section)
    {
      // the relo section we are searching for has this index as sh_info
      size_t target_index = elf_ndxscn(target_section);

      // now iterate over all the sections, look for relocation sections and
      // find the one that points to the section we are searching for
      Elf_Scn*	section = 0;
      GElf_Shdr header_mem, *header;
      while ((section = elf_nextscn(elf_handle, section)) != 0)
	{
	  header = gelf_getshdr(section, &header_mem);
	  if (header == NULL
	      || (header->sh_type != SHT_RELA && header->sh_type != SHT_REL))
	    continue;

	  if (header->sh_info == target_index)
	    return section;
	}
    }
  return NULL;
}

/// Get the version definition (from the SHT_GNU_verdef section) of a
/// given symbol represented by a pointer to GElf_Versym.
///
/// @param elf_hande the elf handle to use.
///
/// @param versym the symbol to get the version definition for.
///
/// @param verdef_section the SHT_GNU_verdef section.
///
/// @param version the resulting version definition.  This is set iff
/// the function returns true.
///
/// @return true upon successful completion, false otherwise.
bool
get_version_definition_for_versym(Elf*			 elf_handle,
				  GElf_Versym*		 versym,
				  Elf_Scn*		 verdef_section,
				  elf_symbol::version&	 version)
{
  Elf_Data* verdef_data = elf_getdata(verdef_section, NULL);
  GElf_Verdef verdef_mem;
  GElf_Verdef* verdef = gelf_getverdef(verdef_data, 0, &verdef_mem);
  size_t vd_offset = 0;

  for (;; vd_offset += verdef->vd_next)
    {
      for (;verdef != 0;)
	{
	  if (verdef->vd_ndx == (*versym & 0x7fff))
	    // Found the version of the symbol.
	    break;
	  vd_offset += verdef->vd_next;
	  verdef = (verdef->vd_next == 0
		    ? 0
		    : gelf_getverdef(verdef_data, vd_offset, &verdef_mem));
	}

      if (verdef != 0)
	{
	  GElf_Verdaux verdaux_mem;
	  GElf_Verdaux *verdaux = gelf_getverdaux(verdef_data,
						  vd_offset + verdef->vd_aux,
						  &verdaux_mem);
	  GElf_Shdr header_mem;
	  GElf_Shdr* verdef_section_header = gelf_getshdr(verdef_section,
							  &header_mem);
	  size_t verdef_stridx = verdef_section_header->sh_link;
	  version.str(elf_strptr(elf_handle, verdef_stridx, verdaux->vda_name));
	  if (*versym & 0x8000)
	    version.is_default(false);
	  else
	    version.is_default(true);
	  return true;
	}
      if (!verdef || verdef->vd_next == 0)
	break;
    }
  return false;
}

/// Get the version needed (from the SHT_GNU_verneed section) to
/// resolve an undefined symbol represented by a pointer to
/// GElf_Versym.
///
/// @param elf_hande the elf handle to use.
///
/// @param versym the symbol to get the version definition for.
///
/// @param verneed_section the SHT_GNU_verneed section.
///
/// @param version the resulting version definition.  This is set iff
/// the function returns true.
///
/// @return true upon successful completion, false otherwise.
bool
get_version_needed_for_versym(Elf*			elf_handle,
			      GElf_Versym*		versym,
			      Elf_Scn*			verneed_section,
			      elf_symbol::version&	version)
{
  if (versym == 0 || elf_handle == 0 || verneed_section == 0)
    return false;

  size_t vn_offset = 0;
  Elf_Data* verneed_data = elf_getdata(verneed_section, NULL);
  GElf_Verneed verneed_mem;
  GElf_Verneed* verneed = gelf_getverneed(verneed_data, 0, &verneed_mem);

  for (;verneed; vn_offset += verneed->vn_next)
    {
      size_t vna_offset = vn_offset;
      GElf_Vernaux vernaux_mem;
      GElf_Vernaux *vernaux = gelf_getvernaux(verneed_data,
					      vn_offset + verneed->vn_aux,
					      &vernaux_mem);
      for (;vernaux != 0 && verneed;)
	{
	  if (vernaux->vna_other == *versym)
	    // Found the version of the symbol.
	    break;
	  vna_offset += verneed->vn_next;
	  verneed = (verneed->vn_next == 0
		     ? 0
		     : gelf_getverneed(verneed_data, vna_offset, &verneed_mem));
	}

      if (verneed != 0 && vernaux != 0 && vernaux->vna_other == *versym)
	{
	  GElf_Shdr header_mem;
	  GElf_Shdr* verneed_section_header = gelf_getshdr(verneed_section,
							   &header_mem);
	  size_t verneed_stridx = verneed_section_header->sh_link;
	  version.str(elf_strptr(elf_handle,
				 verneed_stridx,
				 vernaux->vna_name));
	  if (*versym & 0x8000)
	    version.is_default(false);
	  else
	    version.is_default(true);
	  return true;
	}

      if (!verneed || verneed->vn_next == 0)
	break;
    }
  return false;
}

/// Return the version for a symbol that is at a given index in its
/// SHT_SYMTAB section.
///
/// @param elf_handle the elf handle to use.
///
/// @param symbol_index the index of the symbol to consider.
///
/// @param get_def_version if this is true, it means that that we want
/// the version for a defined symbol; in that case, the version is
/// looked for in a section of type SHT_GNU_verdef.  Otherwise, if
/// this parameter is false, this means that we want the version for
/// an undefined symbol; in that case, the version is the needed one
/// for the symbol to be resolved; so the version is looked fo in a
/// section of type SHT_GNU_verneed.
///
/// @param version the version found for symbol at @p symbol_index.
///
/// @return true iff a version was found for symbol at index @p
/// symbol_index.
bool
get_version_for_symbol(Elf*			elf_handle,
		       size_t			symbol_index,
		       bool			get_def_version,
		       elf_symbol::version&	version)
{
  Elf_Scn *versym_section = NULL,
    *verdef_section = NULL,
    *verneed_section = NULL;

  if (!get_symbol_versionning_sections(elf_handle,
				       versym_section,
				       verdef_section,
				       verneed_section))
    return false;

  GElf_Versym versym_mem;
  Elf_Data* versym_data = (versym_section)
    ? elf_getdata(versym_section, NULL)
    : NULL;
  GElf_Versym* versym = (versym_data)
    ? gelf_getversym(versym_data, symbol_index, &versym_mem)
    : NULL;

  if (versym == 0 || *versym <= 1)
    // I got these value from the code of readelf.c in elfutils.
    // Apparently, if the symbol version entry has these values, the
    // symbol must be discarded. This is not documented in the
    // official specification.
    return false;

  if (get_def_version)
    {
      if (*versym == 0x8001)
	// I got this value from the code of readelf.c in elfutils
	// too.  It's not really documented in the official
	// specification.
	return false;

      if (verdef_section
	  && get_version_definition_for_versym(elf_handle, versym,
					       verdef_section, version))
	return true;
    }
  else
    {
      if (verneed_section
	  && get_version_needed_for_versym(elf_handle, versym,
					   verneed_section, version))
	return true;
    }

  return false;
}

/// Test if the architecture of the current binary is ppc64.
///
/// @param elf_handle the ELF handle to consider.
///
/// @return true iff the architecture of the current binary is ppc64.
bool
architecture_is_ppc64(Elf* elf_handle)
{
  GElf_Ehdr  eh_mem;
  GElf_Ehdr* elf_header = gelf_getehdr(elf_handle, &eh_mem);
  return (elf_header && elf_header->e_machine == EM_PPC64);
}

/// Test if the architecture of the current binary is arm32.
///
/// @param elf_handle the ELF handle to consider.
///
/// @return true iff the architecture of the current binary is arm32.
bool
architecture_is_arm32(Elf* elf_handle)
{
  GElf_Ehdr  eh_mem;
  GElf_Ehdr* elf_header = gelf_getehdr(elf_handle, &eh_mem);
  return (elf_header && elf_header->e_machine == EM_ARM);
}

/// Test if the endianness of the current binary is Big Endian.
///
/// https://en.wikipedia.org/wiki/Endianness.
///
/// @param elf_handle the ELF handle to consider.
///
/// @return true iff the current binary is Big Endian.
bool
architecture_is_big_endian(Elf* elf_handle)
{
  GElf_Ehdr  elf_header;
  gelf_getehdr(elf_handle, &elf_header);

  bool is_big_endian = (elf_header.e_ident[EI_DATA] == ELFDATA2MSB);

  if (!is_big_endian)
    ABG_ASSERT(elf_header.e_ident[EI_DATA] == ELFDATA2LSB);

  return is_big_endian;
}

/// Read N bytes and convert their value into an integer type T.
///
/// Note that N cannot be bigger than 8 for now. The type passed needs to be at
/// least of the size of number_of_bytes.
///
/// @param bytes the array of bytes to read the next 8 bytes from.
/// Note that this array must be at least 8 bytes long.
///
/// @param number_of_bytes the number of bytes to read.  This number
/// cannot be bigger than 8.
///
/// @param is_big_endian if true, read the 8 bytes in Big Endian
/// mode, otherwise, read them in Little Endian.
///
/// @param result where to store the resuting integer that was read.
///
///
/// @param true if the 8 bytes could be read, false otherwise.
template <typename T>
bool
read_int_from_array_of_bytes(const uint8_t* bytes,
			     unsigned char  number_of_bytes,
			     bool	    is_big_endian,
			     T&		    result)
{
  if (!bytes)
    return false;

  ABG_ASSERT(number_of_bytes <= 8);
  ABG_ASSERT(number_of_bytes <= sizeof(T));

  T res = 0;

  const uint8_t* cur = bytes;
  if (is_big_endian)
    {
      // In Big Endian, the most significant byte is at the lowest
      // address.
      const uint8_t* msb = cur;
      res = *msb;

      // Now read the remaining least significant bytes.
      for (uint i = 1; i < number_of_bytes; ++i)
	res = (res << 8) | ((T)msb[i]);
    }
  else
    {
      // In Little Endian, the least significant byte is at the
      // lowest address.
      const uint8_t* lsb = cur;
      res = *lsb;
      // Now read the remaining most significant bytes.
      for (uint i = 1; i < number_of_bytes; ++i)
	res = res | (((T)lsb[i]) << i * 8);
    }

  result = res;
  return true;
}

/// Read 8 bytes and convert their value into an uint64_t.
///
/// @param bytes the array of bytes to read the next 8 bytes from.
/// Note that this array must be at least 8 bytes long.
///
/// @param result where to store the resuting uint64_t that was read.
///
/// @param is_big_endian if true, read the 8 bytes in Big Endian
/// mode, otherwise, read them in Little Endian.
///
/// @param true if the 8 bytes could be read, false otherwise.
bool
read_uint64_from_array_of_bytes(const uint8_t* bytes,
				bool	       is_big_endian,
				uint64_t&      result)
{
  return read_int_from_array_of_bytes(bytes, 8, is_big_endian, result);
}


/// Lookup the address of the function entry point that corresponds
/// to the address of a given function descriptor.
///
/// On PPC64, a function pointer is the address of a function
/// descriptor.  Function descriptors are located in the .opd
/// section.  Each function descriptor is a triplet of three
/// addresses, each one on 64 bits.  Among those three address only
/// the first one is of any interest to us: the address of the entry
/// point of the function.
///
/// This function returns the address of the entry point of the
/// function whose descriptor's address is given.
///
/// http://refspecs.linuxfoundation.org/ELF/ppc64/PPC-elf64abi.html#FUNC-DES
///
/// https://www.ibm.com/developerworks/community/blogs/5894415f-be62-4bc0-81c5-3956e82276f3/entry/deeply_understand_64_bit_powerpc_elf_abi_function_descriptors?lang=en
///
/// @param fn_desc_address the address of the function descriptor to
/// consider.
///
/// @return the address of the entry point of the function whose
/// descriptor has the address @p fn_desc_address.  If there is no
/// .opd section (e.g because we are not on ppc64) or more generally
/// if the function descriptor could not be found then this function
/// just returns the address of the fuction descriptor.
GElf_Addr
lookup_ppc64_elf_fn_entry_point_address(Elf* elf_handle, GElf_Addr fn_desc_address)
{
  if (!elf_handle)
    return fn_desc_address;

  if (!architecture_is_ppc64(elf_handle))
    return fn_desc_address;

  bool is_big_endian = architecture_is_big_endian(elf_handle);

  Elf_Scn* opd_section = find_opd_section(elf_handle);
  if (!opd_section)
    return fn_desc_address;

  GElf_Shdr header_mem;
  // The section header of the .opd section.
  GElf_Shdr* opd_sheader = gelf_getshdr(opd_section, &header_mem);

  // The offset of the function descriptor entry, in the .opd
  // section.
  size_t    fn_desc_offset = fn_desc_address - opd_sheader->sh_addr;
  Elf_Data* elf_data = elf_rawdata(opd_section, 0);

  // Ensure that the opd_section has at least 8 bytes, starting from
  // the offset we want read the data from.
  if (elf_data->d_size <= fn_desc_offset + 8)
    return fn_desc_address;

  // A pointer to the data of the .opd section, that we can actually
  // do something with.
  uint8_t* bytes = (uint8_t*)elf_data->d_buf;

  // The resulting address we are looking for is going to be formed
  // in this variable.
  GElf_Addr result = 0;
  ABG_ASSERT(read_uint64_from_array_of_bytes(bytes + fn_desc_offset,
					     is_big_endian, result));

  return result;
}

/// Test if the ELF binary denoted by a given ELF handle is a Linux
/// Kernel Module.
///
/// @param elf_handle the ELF handle to consider.
///
/// @return true iff the binary denoted by @p elf_handle is a Linux
/// kernel module.
bool
is_linux_kernel_module(Elf *elf_handle)
{
  return (find_section(elf_handle, ".modinfo", SHT_PROGBITS)
	  && find_section(elf_handle,
			  ".gnu.linkonce.this_module",
			  SHT_PROGBITS));
}

/// Test if the ELF binary denoted by a given ELF handle is a Linux
/// Kernel binary (either vmlinux or a kernel module).
///
/// @param elf_handle the ELF handle to consider.
///
/// @return true iff the binary denoted by @p elf_handle is a Linux
/// kernel binary
bool
is_linux_kernel(Elf *elf_handle)
{
  return (find_section(elf_handle,
		       "__ksymtab_strings",
		       SHT_PROGBITS)
	  || is_linux_kernel_module(elf_handle));
}

/// Get the address at which a given binary is loaded in memory.
///
/// @param elf_handle the elf handle for the binary to consider.
///
/// @param load_address the address where the binary is loaded.  This
/// is set by the function iff it returns true.
///
/// @return true if the function could get the binary load address
/// and assign @p load_address to it.
bool
get_binary_load_address(Elf* elf_handle, GElf_Addr& load_address)
{
  GElf_Ehdr elf_header;
  gelf_getehdr(elf_handle, &elf_header);
  size_t num_segments = elf_header.e_phnum;
  GElf_Phdr *program_header = NULL;
  GElf_Addr result;
  bool found_loaded_segment = false;
  GElf_Phdr ph_mem;

  for (unsigned i = 0; i < num_segments; ++i)
    {
      program_header = gelf_getphdr(elf_handle, i, &ph_mem);
      if (program_header && program_header->p_type == PT_LOAD)
	{
	  if (!found_loaded_segment)
	    {
	      result = program_header->p_vaddr;
	      found_loaded_segment = true;
	    }

	  if (program_header->p_vaddr < result)
	    // The resulting load address we want is the lowest
	    // load address of all the loaded segments.
	    result = program_header->p_vaddr;
	}
    }

  if (found_loaded_segment)
    {
      load_address = result;
      return true;
    }
  return false;
}

/// Return the size of a word for the current architecture.
///
/// @param elf_handle the ELF handle to consider.
///
/// @return the size of a word.
unsigned char
get_architecture_word_size(Elf* elf_handle)
{
  unsigned char word_size = 0;
  GElf_Ehdr	elf_header;
  gelf_getehdr(elf_handle, &elf_header);
  if (elf_header.e_ident[EI_CLASS] == ELFCLASS32)
    word_size = 4;
  else if (elf_header.e_ident[EI_CLASS] == ELFCLASS64)
    word_size = 8;
  else
    ABG_ASSERT_NOT_REACHED;
  return word_size;
}

/// Test if the elf file being read is an executable.
///
/// @param elf_handle the ELF handle to consider.
///
/// @return true iff the elf file being read is an / executable.
bool
is_executable(Elf* elf_handle)
{
  GElf_Ehdr  elf_header;
  gelf_getehdr(elf_handle, &elf_header);
  return elf_header.e_type == ET_EXEC;
}

/// Test if the elf file being read is a dynamic shared / object.
///
/// @param elf_handle the ELF handle to consider.
///
/// @return true iff the elf file being read is a / dynamic shared object.
bool
is_dso(Elf* elf_handle)
{
  GElf_Ehdr  elf_header;
  gelf_getehdr(elf_handle, &elf_header);
  return elf_header.e_type == ET_DYN;
}

/// Translate a section-relative symbol address (i.e, symbol value)
/// into an absolute symbol address by adding the address of the
/// section the symbol belongs to, to the address value.
///
/// This is useful when looking at symbol values coming from
/// relocatable files (of ET_REL kind).  If the binary is not
/// ET_REL, then the function does nothing and returns the input
/// address unchanged.
///
/// @param elf_handle the elf handle for the binary to consider.
///
/// @param sym the symbol whose address to possibly needs to be
/// translated.
///
/// @return the section-relative address, translated into an
/// absolute address, if @p sym is from an ET_REL binary.
/// Otherwise, return the address of @p sym, unchanged.
GElf_Addr
maybe_adjust_et_rel_sym_addr_to_abs_addr(Elf* elf_handle, GElf_Sym* sym)
{
  Elf_Scn*  symbol_section = elf_getscn(elf_handle, sym->st_shndx);
  GElf_Addr addr = sym->st_value;

  if (!symbol_section)
    return addr;

  GElf_Ehdr elf_header;
  if (!gelf_getehdr(elf_handle, &elf_header))
    return addr;

  if (elf_header.e_type != ET_REL)
    return addr;

  GElf_Shdr section_header;
  if (!gelf_getshdr(symbol_section, &section_header))
    return addr;

  return addr + section_header.sh_addr;
}

/// Test if a given address is in a given section.
///
/// @param addr the address to consider.
///
/// @param section the section to consider.
///
/// @return true iff @p addr is in section @p section.
bool
address_is_in_section(Dwarf_Addr addr, Elf_Scn* section)
{
  if (!section)
    return false;

  GElf_Shdr  sheader_mem;
  GElf_Shdr* sheader = gelf_getshdr(section, &sheader_mem);

  if (sheader->sh_addr <= addr && addr <= sheader->sh_addr + sheader->sh_size)
    return true;

  return false;
}

/// Return true if an address is in the ".opd" section that is
/// present on the ppc64 platform.
///
/// @param addr the address to consider.
///
/// @return true iff @p addr designates a word that is in the ".opd"
/// section.
bool
address_is_in_opd_section(Elf* elf_handle, Dwarf_Addr addr)
{
  Elf_Scn * opd_section = find_opd_section(elf_handle);
  if (!opd_section)
    return false;
  if (address_is_in_section(addr, opd_section))
    return true;
  return false;
}


} // end namespace elf_helpers
} // end namespace abigail
