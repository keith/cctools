#include <stdio.h>
#include <string.h>
#include <mach-o/loader.h>
#include <stuff/bool.h>
#include <stuff/allocate.h>
#include "dyld_bind_info.h"

const char *
bindTypeName(
uint8_t type)
{
        switch(type){
	    case BIND_TYPE_POINTER:
		return("pointer");
	    case BIND_TYPE_TEXT_ABSOLUTE32:
		return("text abs32");
	    case BIND_TYPE_TEXT_PCREL32:
		return("text rel32");
        }
        return("!!Unknown!!");
}

const char *
ordinalName(
int libraryOrdinal,
const char **dylibs,
uint32_t ndylibs,
enum bool *libraryOrdinalSet)
{
	*libraryOrdinalSet = TRUE;
        switch(libraryOrdinal){
	    case BIND_SPECIAL_DYLIB_SELF:
		return("this-image");
	    case BIND_SPECIAL_DYLIB_MAIN_EXECUTABLE:
		return("main-executable");
	    case BIND_SPECIAL_DYLIB_FLAT_LOOKUP:
		return("flat-namespace");
        }
        if(libraryOrdinal < BIND_SPECIAL_DYLIB_FLAT_LOOKUP){
	    *libraryOrdinalSet = FALSE;
	    return("Unknown special ordinal");
	}
        if(libraryOrdinal > ndylibs){
	    *libraryOrdinalSet = FALSE;
	    return("LibraryOrdinal out of range");
	}
        return(dylibs[libraryOrdinal-1]);
}

uint64_t
segStartAddress(
uint8_t segIndex,
struct segment_command **segs,
uint32_t nsegs,
struct segment_command_64 **segs64,
uint32_t nsegs64)
{
	if(segs != NULL){
	    if(segIndex >= nsegs)
		return(0); /* throw "segment index out of range"; */
	    return(segs[segIndex]->vmaddr);
	}
	else if(segs64 != NULL){
	    if(segIndex >= nsegs64)
		return(0); /* throw "segment index out of range"; */
	    return(segs64[segIndex]->vmaddr);
	}
	return(0);
}

const char *
segmentName(
uint8_t segIndex,
struct segment_command **segs,
uint32_t nsegs,
struct segment_command_64 **segs64,
uint32_t nsegs64)
{
	if(segs != NULL){
	    if(segIndex >= nsegs)
		return("??"); /* throw "segment index out of range"; */
	    return(segs[segIndex]->segname);
	}
	else if(segs64 != NULL){
	    if(segIndex >= nsegs64)
		return("??"); /* throw "segment index out of range"; */
	    return(segs64[segIndex]->segname);
	}
	return("??");
}

const char *
sectionName(
uint8_t segIndex,
uint64_t address,
struct segment_command **segs,
uint32_t nsegs,
struct segment_command_64 **segs64,
uint32_t nsegs64)
{
    struct section *s;
    struct section_64 *s64;
    uint32_t i;

	if(segs != NULL){
	    if(segIndex >= nsegs)
		return("??"); /* throw "segment index out of range"; */
	    
	    s = (struct section *)((char *)segs[segIndex] + 
				   sizeof(struct segment_command));
	    for(i = 0; i < segs[segIndex]->nsects; i++){
		if(s->addr <= address && address < s->addr + s->size)
		    return(s->sectname);
		s++;
	    }
	}
	else if(segs64 != NULL){
	    if(segIndex >= nsegs64)
		return("??"); /* throw "segment index out of range"; */
	    
	    s64 = (struct section_64 *)((char *)segs64[segIndex] + 
					sizeof(struct segment_command_64));
	    for(i = 0; i < segs64[segIndex]->nsects; i++){
		if(s64->addr <= address && address < s64->addr + s64->size)
		    return(s64->sectname);
		s64++;
	    }
	}
	return("??");
}

const char *
checkSegAndOffset(
int segIndex,
uint64_t segOffset,
struct segment_command **segs,
uint32_t nsegs,
struct segment_command_64 **segs64,
uint32_t nsegs64,
enum bool endInvalid)
{
    uint64_t address;

	if(segIndex == -1)
	    return("missing preceding BIND_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB");
	if(segs != NULL){
	    if(segIndex >= nsegs)
		return("bad segIndex (too large)");
	    address = segs[segIndex]->vmaddr + segOffset;
	    if(address > segs[segIndex]->vmaddr + segs[segIndex]->vmsize)
		return("bad segOffset, too large");
	    if(endInvalid == TRUE &&
	       address == segs[segIndex]->vmaddr + segs[segIndex]->vmsize)
		return("bad segOffset, too large");
	}
	else if(segs64 != NULL){
	    if(segIndex >= nsegs64)
		return("bad segIndex (too large)");
	    address = segs64[segIndex]->vmaddr + segOffset;
	    if(address > segs64[segIndex]->vmaddr + segs64[segIndex]->vmsize)
		return("bad segOffset, too large");
	    if(endInvalid == TRUE &&
	       address == segs64[segIndex]->vmaddr + segs64[segIndex]->vmsize)
		return("bad segOffset, too large");
	}
	return(NULL);
}

const char *
checkCountAndSkip(
uint32_t *count,
uint64_t skip,
int segIndex,
uint64_t segOffset,
struct segment_command **segs,
uint32_t nsegs,
struct segment_command_64 **segs64,
uint32_t nsegs64)
{
    uint64_t address, i;

	i = 0;
	if(segs != NULL){
	    if(segIndex >= nsegs){
		*count = 1;
		return("bad segIndex (too large)");
	    }
	    if(*count > 1)
		i = (skip + 4) * (*count - 1);
	    address = segs[segIndex]->vmaddr + segOffset;
	    if(address >= segs[segIndex]->vmaddr + segs[segIndex]->vmsize){
		*count = 1;
		return("bad segOffset, too large");
	    }
	    if(address + i >= segs[segIndex]->vmaddr + segs[segIndex]->vmsize){
		*count = 1;
		return("bad count and skip, too large");
	    }
	}
	else if(segs64 != NULL){
	    if(segIndex >= nsegs64){
		*count = 1;
		return("bad segIndex (too large)");
	    }
	    if(*count > 1)
		i = (skip + 8) * (*count - 1);
	    address = segs64[segIndex]->vmaddr + segOffset;
	    if(address >= segs64[segIndex]->vmaddr + segs64[segIndex]->vmsize){
		*count = 1;
		return("bad segOffset, too large");
	    }
	    if(address + i >=
	       segs64[segIndex]->vmaddr + segs64[segIndex]->vmsize){
		*count = 1;
		return("bad count and skip, too large");
	    }
	}
	return(NULL);
}

static
uint64_t
read_uleb128(
const uint8_t **pp,
const uint8_t* end,
const char **error)
{
    const uint8_t *p = *pp;
    uint64_t result = 0;
    int bit = 0;

	*error = NULL;
        do{
	    if(p == end){
		*pp = p;
		*error = "malformed uleb128, extends past opcode bytes";
		return(0);
	    }

	    uint64_t slice = *p & 0x7f;

	    if(bit >= 64 || slice << bit >> bit != slice){
		*pp = p;
		*error = "uleb128 too big for uint64";
		return(0);
	    }
	    else {
		result |= (slice << bit);
		bit += 7;
	    }
        }while(*p++ & 0x80);
	*pp = p;
        return(result);
}

static
int64_t
read_sleb128(
const uint8_t **pp,
const uint8_t* end,
const char **error)
{
    const uint8_t *p = *pp;
    int64_t result = 0;
    int bit = 0;
    uint8_t byte;

	*error = NULL;
        do{
	    if(p == end){
		*pp = p;
		*error = "malformed sleb128, extends past opcode bytes";
		return(0);
	    }
	    byte = *p++;
	    result |= (((int64_t)(byte & 0x7f)) << bit);
	    bit += 7;
        }while (byte & 0x80);
        // sign extend negative numbers
        if((byte & 0x40) != 0)
	    result |= (-1LL) << bit;
	*pp = p;
        return(result);
}

/*
 * get_dyld_bind_info() unpacks the dyld bind info from the data from an
 * LC_BIND_INFO load command pointed to by start through end into the internal
 * dyld_bind_info structs returned through dbi, and its count ndbi.  The array
 * of dylib names and their count are passed in in dylibs and ndylibs.  The
 * array of segments (either 32-bit with segs & nsegs or 64-bit segs64 & nsegs64)
 * are used to determine which sections the pointers are in.
 */
void
get_dyld_bind_info(
const uint8_t *start, /* inputs */
const uint8_t* end,
const char **dylibs,
uint32_t ndylibs,
struct segment_command **segs,
uint32_t nsegs,
struct segment_command_64 **segs64,
uint32_t nsegs64,
struct dyld_bind_info **dbi, /* output */
uint64_t *ndbi,
enum bool print_errors)
{
    const uint8_t *p, *opcode_start;
    uint8_t type;
    int segIndex;
    uint64_t segOffset;
    const char* symbolName;
    const char* fromDylib;
    enum bool libraryOrdinalSet;
    int libraryOrdinal;
    int64_t addend;
    uint32_t count;
    uint32_t skip;
    uint64_t segStartAddr;
    const char* segName;
    const char* typeName;
    const char* weak_import;
    enum bool done = FALSE;

    const char *sectName, *error;
#define MAXERRORCOUNT 20
    uint32_t pass, errorCount;
    uint64_t n;

    uint32_t sizeof_pointer;
    if(segs)
	sizeof_pointer = 4;
    else
	sizeof_pointer = 8;

    errorCount = 0;
    n = 0;
    for(pass = 1; pass <= 2; pass++){
	p = start;
	type = 0;
        segIndex = -1;
	segOffset = 0;
	symbolName = NULL;
	fromDylib = "??";
	libraryOrdinalSet = FALSE;
	libraryOrdinal = 0;
	addend = 0;
	segStartAddr = 0;
	segName = "??";
	typeName = "??";
	weak_import = "";
	done = FALSE;
	if(errorCount >= MAXERRORCOUNT){
	    if(print_errors)
		printf("too many bind info errors\n");
	    *dbi = NULL;
	    *ndbi = 0;
	    return;
	}
	if(pass == 2){
	    *dbi = (struct dyld_bind_info *)
		  allocate(n * sizeof(struct dyld_bind_info));
	    *ndbi = n;
	    n = 0;
	}
	while(!done && (p < end) && errorCount < MAXERRORCOUNT){
	    error = NULL;
	    opcode_start = p;
	    uint8_t immediate = *p & BIND_IMMEDIATE_MASK;
	    uint8_t opcode = *p & BIND_OPCODE_MASK;
	    opcode = *p & BIND_OPCODE_MASK;
	    ++p;
	    switch(opcode){
		case BIND_OPCODE_DONE:
		    done = TRUE;
		    break;
		case BIND_OPCODE_SET_DYLIB_ORDINAL_IMM:
		    libraryOrdinal = immediate;
		    if(pass == 1 && print_errors && immediate > ndylibs){
			printf("bad bind info (for BIND_OPCODE_SET_DYLIB_"
			       "ORDINAL_ULEB bad library ordinal: %u (max %u) "
			       "for opcode at: 0x%lx)\n", immediate, ndylibs,
			       opcode_start - start);
			errorCount++;
		    }
		    fromDylib = ordinalName(libraryOrdinal, dylibs, ndylibs,
					    &libraryOrdinalSet);
		    break;
		case BIND_OPCODE_SET_DYLIB_ORDINAL_ULEB:
		    libraryOrdinal = read_uleb128(&p, end, &error);
		    if(pass == 1 && print_errors && error) {
			printf("bad bind info (for BIND_OPCODE_SET_DYLIB_"
			       "ORDINAL_ULEB %s for opcode at: 0x%lx)\n",
			       error, opcode_start - start);
			errorCount++;
		    }
		    if(pass == 1 && print_errors && libraryOrdinal > ndylibs){
			printf("bad bind info (for BIND_OPCODE_SET_DYLIB_"
			       "ORDINAL_ULEB bad library ordinal: %u (max %u) "
			       "for opcode at: 0x%lx)\n", libraryOrdinal,
			       ndylibs, opcode_start - start);
			errorCount++;
		    }
		    fromDylib = ordinalName(libraryOrdinal, dylibs, ndylibs,
					    &libraryOrdinalSet);
		    break;
		case BIND_OPCODE_SET_DYLIB_SPECIAL_IMM:
		    // the special ordinals are negative numbers
		    if(immediate == 0)
			libraryOrdinal = 0;
		    else {
			int8_t signExtended = BIND_OPCODE_MASK | immediate;
			libraryOrdinal = signExtended;
			if(pass == 1 && print_errors &&
                           libraryOrdinal < BIND_SPECIAL_DYLIB_FLAT_LOOKUP){
			    printf("bad bind info (for BIND_OPCODE_SET_DYLIB_"
				   "SPECIAL_IMM unknown special ordinal: %d "
				   "for opcode at: 0x%lx)\n", libraryOrdinal,
				   opcode_start - start);
			    errorCount++;
			}
		    }
		    fromDylib = ordinalName(libraryOrdinal, dylibs, ndylibs,
					    &libraryOrdinalSet);
		    break;
		case BIND_OPCODE_SET_SYMBOL_TRAILING_FLAGS_IMM:
		    symbolName = (char*)p;
		    while(*p != '\0' && (p < end))
			++p;
		    if(p == end){
			if(pass == 1 && print_errors){
			    printf("bad bind info (for BIND_OPCODE_SET_SYMBOL_"
				   "TRAILING_FLAGS_IMM symbol name extends "
				   "past opcodes for opcode at: 0x%lx)\n",
				   opcode_start - start);
			    errorCount++;
			}
			/*
			 * Even though the name does not end with a '\0' it will
			 * not be used as it is past the opcodes so there can't
			 * be a BIND opcode that follows that will use it.
			 */
		    }
		    else {
			++p;
		    }
		    if((immediate & BIND_SYMBOL_FLAGS_WEAK_IMPORT) != 0)
			weak_import = " (weak import)";
		    else
			weak_import = "";
		    break;
		case BIND_OPCODE_SET_TYPE_IMM:
		    if(pass == 1 && print_errors &&
		       (immediate == 0 || immediate > BIND_TYPE_TEXT_PCREL32)){
			printf("bad bind info (for BIND_OPCODE_SET_TYPE_IMM "
			       "bad bind type: %u) for opcode at: 0x%lx)\n",
			       immediate, opcode_start - start);
			errorCount++;
		    }
		    type = immediate;
		    typeName = bindTypeName(type);
		    break;
		case BIND_OPCODE_SET_ADDEND_SLEB:
		    addend = read_sleb128(&p, end, &error);
		    if(pass == 1 && print_errors && error){
			printf("bad bind info (for BIND_OPCODE_SET_ADDEND_SLEB "
			       "%s for opcode at: 0x%lx)\n", error,
			       opcode_start - start);
			errorCount++;
		    }
		    break;
		case BIND_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB:
		    segIndex = immediate;
		    segStartAddr = segStartAddress(segIndex, segs, nsegs,
						   segs64, nsegs64);
		    segName = segmentName(segIndex, segs, nsegs,
					  segs64, nsegs64);
		    segOffset = read_uleb128(&p, end, &error);
		    if(pass == 1 && print_errors && error){
			printf("bad bind info (for BIND_OPCODE_SET_SEGMENT_AND_"
			       "OFFSET_ULEB %s for opcode at: 0x%lx)\n",
			       error, opcode_start - start);
			errorCount++;
		    }
		    error = checkSegAndOffset(segIndex, segOffset, segs, nsegs,
					      segs64, nsegs64, TRUE);
		    if(pass == 1 && print_errors && error){
			printf("bad bind info (for BIND_OPCODE_SET_SEGMENT_AND_"
			       "OFFSET_ULEB %s for opcode at: 0x%lx)\n",
			       error, opcode_start - start);
			errorCount++;
		    }
		    break;
		case BIND_OPCODE_ADD_ADDR_ULEB:
		    segOffset += read_uleb128(&p, end, &error);
		    if(pass == 1 && print_errors && error){
			printf("bad bind info (for BIND_OPCODE_ADD_ADDR_ULEB "
			       "%s for opcode at: 0x%lx)\n", error,
			       opcode_start - start);
			errorCount++;
		    }
		    error = checkSegAndOffset(segIndex, segOffset, segs, nsegs,
					      segs64, nsegs64, TRUE);
		    if(pass == 1 && print_errors && error){
			printf("bad bind info (for BIND_OPCODE_ADD_ADDR_ULEB "
			       "%s for opcode at: 0x%lx)\n", error,
			       opcode_start - start);
			errorCount++;
		    }
		    break;
		case BIND_OPCODE_DO_BIND:
		    error = checkSegAndOffset(segIndex, segOffset, segs, nsegs,
					      segs64, nsegs64, TRUE);
		    if(pass == 1 && print_errors && error){
			printf("bad bind info (for BIND_OPCODE_DO_BIND %s for "
			       "opcode at: 0x%lx)\n", error,
			       opcode_start - start);
			errorCount++;
		    }
		    sectName = sectionName(segIndex, segStartAddr + segOffset,
					   segs, nsegs, segs64, nsegs64);
		    if(pass == 1 && print_errors && symbolName == NULL){
			printf("bad bind info (for BIND_OPCODE_DO_BIND missing "
			       "preceding BIND_OPCODE_SET_SYMBOL_TRAILING_"
			       "FLAGS_IMM for opcode at: 0x%lx)\n",
			       opcode_start - start);
			errorCount++;
		    }
		    if(pass == 1 && print_errors && libraryOrdinalSet == FALSE){
			printf("bad bind info (for BIND_OPCODE_DO_BIND missing "
			       "preceding BIND_OPCODE_SET_DYLIB_ORDINAL_* for "
			       "opcode at: 0x%lx)\n", opcode_start - start);
			errorCount++;
		    }
		    if(pass == 2){
			(*dbi)[n].segname = segName;
			(*dbi)[n].sectname = sectName;
			(*dbi)[n].address = segStartAddr+segOffset;
			(*dbi)[n].bind_type = type;
			(*dbi)[n].addend = addend;
			(*dbi)[n].dylibname = fromDylib;
			(*dbi)[n].symbolname = symbolName ? symbolName :
					       "Symbol name not set";
			(*dbi)[n].weak_import = *weak_import != '\0';
		    }
		    n++;
		    segOffset += sizeof_pointer;
		    break;
		case BIND_OPCODE_DO_BIND_ADD_ADDR_ULEB:
		    error = checkSegAndOffset(segIndex, segOffset, segs, nsegs,
					      segs64, nsegs64, TRUE);
		    if(pass == 1 && print_errors && error){
			printf("bad bind info (for BIND_OPCODE_DO_BIND_ADD_"
			       "ADDR_ULEB %s for opcode at: 0x%lx)\n", error,
			       opcode_start - start);
			errorCount++;
		    }
		    sectName = sectionName(segIndex, segStartAddr + segOffset,
					   segs, nsegs, segs64, nsegs64);
		    if(pass == 1 && print_errors && symbolName == NULL){
			printf("bad bind info (for BIND_OPCODE_DO_BIND_ADD_"
			       "ADDR_ULEB missing preceding BIND_OPCODE_SET_"
			       "SYMBOL_TRAILING_FLAGS_IMM for opcode at: "
			       "0x%lx)\n", opcode_start - start);
			errorCount++;
		    }
		    if(pass == 1 && print_errors && libraryOrdinalSet == FALSE){
			printf("bad bind info (for BIND_OPCODE_DO_BIND_ADD_"
			       "ADDR_ULEB missing preceding BIND_OPCODE_SET_"
			       "DYLIB_ORDINAL_* for opcode at: 0x%lx)\n",
			       opcode_start - start);
			errorCount++;
		    }
		    if(pass == 2){
			(*dbi)[n].segname = segName;
			(*dbi)[n].sectname = sectName;
			(*dbi)[n].address = segStartAddr+segOffset;
			(*dbi)[n].bind_type = type;
			(*dbi)[n].addend = addend;
			(*dbi)[n].dylibname = fromDylib;
			(*dbi)[n].symbolname = symbolName ? symbolName :
					       "Symbol name not set";
			(*dbi)[n].weak_import = *weak_import != '\0';
		    }
		    n++;
		    segOffset += read_uleb128(&p, end, &error) + sizeof_pointer;
		    if(pass == 1 && print_errors && error){
			printf("bad bind info (for BIND_OPCODE_DO_BIND_ADD_"
			       "ADDR_ULEB %s for opcode at: 0x%lx)\n", error,
			       opcode_start - start);
			errorCount++;
		    }
		    /*
		     * Note, this is not really an error until the next bind
		     * but make so sense for a BIND_OPCODE_DO_BIND_ADD_ADDR_ULEB
		     * to not be followed by another bind operation.
		     */
		    error = checkSegAndOffset(segIndex, segOffset, segs, nsegs,
					      segs64, nsegs64, FALSE);
		    if(pass == 1 && print_errors && error){
			printf("bad bind info (for BIND_OPCODE_DO_BIND_ADD_"
			       "ADDR_ULEB (after adding ULEB) %s for opcode "
			       "at: 0x%lx)\n", error, opcode_start - start);
			errorCount++;
		    }
		    break;
		case BIND_OPCODE_DO_BIND_ADD_ADDR_IMM_SCALED:
		    error = checkSegAndOffset(segIndex, segOffset, segs, nsegs,
					      segs64, nsegs64, TRUE);
		    if(pass == 1 && print_errors && error){
			printf("bad bind info (for BIND_OPCODE_DO_BIND_ADD_"
                               "ADDR_IMM_SCALED %s for opcode at: 0x%lx)\n",
			       error, opcode_start - start);
			errorCount++;
		    }
		    sectName = sectionName(segIndex, segStartAddr + segOffset,
					   segs, nsegs, segs64, nsegs64);
		    if(pass == 1 && print_errors && symbolName == NULL){
			printf("bad bind info (for BIND_OPCODE_DO_BIND_ADD_"
			       "ADDR_IMM_SCALED missing preceding BIND_OPCODE_"
			       "SET_SYMBOL_TRAILING_FLAGS_IMM for opcode at: "
			       "0x%lx)\n", opcode_start - start);
			errorCount++;
		    }
		    if(pass == 1 && print_errors && libraryOrdinalSet == FALSE){
			printf("bad bind info (for BIND_OPCODE_DO_BIND_ADD_"
			       "ADDR_IMM_SCALED missing preceding "
			       "BIND_OPCODE_SET_DYLIB_ORDINAL_* for opcode at: "
			       "0x%lx)\n", opcode_start - start);
			errorCount++;
		    }
		    if(pass == 2){
			(*dbi)[n].segname = segName;
			(*dbi)[n].sectname = sectName;
			(*dbi)[n].address = segStartAddr+segOffset;
			(*dbi)[n].bind_type = type;
			(*dbi)[n].addend = addend;
			(*dbi)[n].dylibname = fromDylib;
			(*dbi)[n].symbolname = symbolName ? symbolName :
					       "Symbol name not set";
			(*dbi)[n].weak_import = *weak_import != '\0';
		    }
		    n++;
		    segOffset += immediate * sizeof_pointer + sizeof_pointer;
		    /*
		     * Note, this is not really an error until the next bind
		     * but make so sense for a BIND_OPCODE_DO_BIND_ADD_ADDR_ULEB
		     * to not be followed by another bind operation.
		     */
		    error = checkSegAndOffset(segIndex, segOffset, segs, nsegs,
					      segs64, nsegs64, FALSE);
		    if(pass == 1 && print_errors && error){
			printf("bad bind info (for BIND_OPCODE_DO_BIND_ADD_"
                               "ADDR_IMM_SCALED (after adding immediate times "
			       "the pointer size) %s for opcode at: 0x%lx)\n",
			       error, opcode_start - start);
			errorCount++;
		    }
		    break;
		case BIND_OPCODE_DO_BIND_ULEB_TIMES_SKIPPING_ULEB:
		    count = read_uleb128(&p, end, &error);
		    if(pass == 1 && print_errors && error){
			printf("bad bind info (for BIND_OPCODE_DO_BIND_ULEB_"
			       "TIMES_SKIPPING_ULEB (count value) %s for "
			       "opcode at: 0x%lx)\n", error,
			       opcode_start - start);
			errorCount++;
		    }
		    skip = read_uleb128(&p, end, &error);
		    if(pass == 1 && print_errors && error){
			printf("bad bind info (for BIND_OPCODE_DO_BIND_ULEB_"
			       "TIMES_SKIPPING_ULEB (skip value) %s for "
			       "opcode at: 0x%lx)\n", error,
			       opcode_start - start);
			errorCount++;
		    }
		    error = checkSegAndOffset(segIndex, segOffset, segs, nsegs,
					      segs64, nsegs64, TRUE);
		    if(pass == 1 && print_errors && error){
			printf("bad bind info (for BIND_OPCODE_DO_BIND_ULEB_"
			       "TIMES_SKIPPING_ULEB %s for opcode at: 0x%lx)\n",
			       error, opcode_start - start);
			errorCount++;
		    }
		    sectName = sectionName(segIndex, segStartAddr + segOffset,
					   segs, nsegs, segs64, nsegs64);
		    if(pass == 1 && print_errors && symbolName == NULL){
			printf("bad bind info (for BIND_OPCODE_DO_BIND_ULEB_"
			       "TIMES_SKIPPING_ULEB missing preceding BIND_"
			       "OPCODE_SET_SYMBOL_TRAILING_FLAGS_IMM for "
			       "opcode at: 0x%lx)\n", opcode_start - start);
			errorCount++;
		    }
		    if(pass == 1 && print_errors && libraryOrdinalSet == FALSE){
			printf("bad bind info (for BIND_OPCODE_DO_BIND_ULEB_"
			       "TIMES_SKIPPING_ULEB missing preceding BIND_"
			       "OPCODE_SET_DYLIB_ORDINAL_* for opcode at: "
			       "0x%lx)\n", opcode_start - start);
			errorCount++;
		    }
		    error = checkCountAndSkip(&count, skip, segIndex,
				    segOffset, segs, nsegs, segs64, nsegs64);
		    if(pass == 1 && print_errors && error){
			printf("bad bind info (for BIND_OPCODE_DO_BIND_ULEB_"
			       "TIMES_SKIPPING_ULEB %s for opcode at: 0x%lx)\n",
			       error, opcode_start - start);
			errorCount++;
		    }
		    for (uint32_t i=0; i < count; ++i) {
			if(pass == 2){
			    (*dbi)[n].segname = segName;
			    (*dbi)[n].sectname = sectName;
			    (*dbi)[n].address = segStartAddr+segOffset;
			    (*dbi)[n].bind_type = type;
			    (*dbi)[n].addend = addend;
			    (*dbi)[n].dylibname = fromDylib;
			    (*dbi)[n].symbolname = symbolName ? symbolName :
					           "Symbol name not set";
			    (*dbi)[n].weak_import = *weak_import != '\0';
			}
			n++;
			segOffset += skip + sizeof_pointer;
		    }
		    break;
		default:
		    if(pass == 1 && print_errors){
			printf("bad bind info (bad opcode value 0x%x for "
			       "opcode at: 0x%lx)\n", opcode,
			       opcode_start - start);
			errorCount++;
		    }
		    done = TRUE;
		    break;
	    }
	}	
    }
}

/*
 * print_dyld_bind_info() prints the internal expanded dyld bind information in
 * the same format as dyldinfo(1)'s -bind option.
 */
void
print_dyld_bind_info(
struct dyld_bind_info *dbi,
uint64_t ndbi)
{
    uint64_t n;

	printf("bind information:\n");
	printf("segment section          address        type    addend dylib"
	       "            symbol\n");
	for(n = 0; n < ndbi; n++){
	    printf("%-7s %-16.16s 0x%08llX %10s  %5lld %-16s %s%s\n",
		dbi[n].segname,
		dbi[n].sectname,
		dbi[n].address,
		bindTypeName(dbi[n].bind_type),
		dbi[n].addend,
		dbi[n].dylibname,
		dbi[n].symbolname,
		dbi[n].weak_import ? " (weak import)" : "");
	}
}

/*
 * get_dyld_bind_info_symbolname() is passed an address and the internal
 * expanded dyld bind information.  If the address is found its binding symbol
 * name is returned.  If not NULL.
 */
const char *
get_dyld_bind_info_symbolname(
uint64_t address,
struct dyld_bind_info *dbi,
uint64_t ndbi)
{
    uint64_t n;

	for(n = 0; n < ndbi; n++){
	    if(dbi[n].address == address)
		return(dbi[n].symbolname);
	}
	return(NULL);
}
