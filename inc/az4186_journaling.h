#ifndef def_az4186_journaling_h_included
    #define def_az4186_journaling_h_included

#define def_enable_journaling

#ifdef def_enable_journaling

typedef enum{
	enum_open_journal_sequence_retcode_ok=0,
	enum_open_journal_sequence_retcode_already_opened,
	enum_open_journal_sequence_retcode_numof
}enum_open_journal_sequence_retcode;

// opens a journal sequence
// this routine should be cqalled before initiating a write to file sequence
//
enum_open_journal_sequence_retcode ui_open_journal_sequence(void);

typedef enum{
	enum_close_journal_sequence_retcode_ok=0,
	enum_close_journal_sequence_retcode_not_opened,
	enum_close_journal_sequence_retcode_numof
}enum_close_journal_sequence_retcode;

// closes a journal sequence
enum_close_journal_sequence_retcode ui_close_journal_sequence(void);


typedef enum{
	enum_commit_journal_sequence_retcode_ok=0,
	enum_commit_journal_sequence_retcode_bad_magic_word,
    enum_commit_journal_sequence_retcode_bad_crc,
    enum_commit_journal_sequence_retcode_disk_io_err,
	enum_commit_journal_sequence_retcode_numof
}enum_commit_journal_sequence_retcode;

// commit the journal queue
enum_commit_journal_sequence_retcode ui_commit_journal_sequence(void);

// initializes the journal sequence
typedef enum{
	enum_init_journal_retcode_ok=0,
	enum_init_journal_retcode_check_error_unresolved,
	enum_init_journal_retcode_check_error_solved,
	enum_init_journal_retcode_commit_error,
	enum_init_journal_retcode_numof
}enum_init_journal_retcode;

// initialize the sd journal and complete a possible previously-suspended commit
// this routine should be called when initializing the system, just after the sd card driver has been mounted and initialized
enum_init_journal_retcode ui_init_sd_journal(void);

typedef enum{
	enum_copy_sector_journal_sequence_retcode_ok=0,
	enum_copy_sector_journal_sequence_retcode_not_found,
	enum_copy_sector_journal_sequence_retcode_bad_queue,
	enum_copy_sector_journal_sequence_retcode_bad_crc,
	enum_copy_sector_journal_sequence_retcode_numof
}enum_copy_sector_journal_sequence_retcode;

// retcodes for close and commit journal routine
typedef enum{
	enum_close_and_commit_journal_sequence_retcode_ok=0,
	enum_close_and_commit_journal_sequence_retcode_ko,
	enum_close_and_commit_journal_sequence_retcode_numof,
}enum_close_and_commit_journal_sequence_retcode;

// this routine closes and commits a journal sequence
enum_close_and_commit_journal_sequence_retcode ui_close_and_commit_journal_sequence(void);

// check if the journaling cache contains the block required
//
// input:
//   drive: the drive where read from
//   buff : the destination buffer where to copy the cached data
//   ui_sector: the start sector read number
//   uc_num_of_sector_to_read: the number of sector to read
//
// returns:
//    0 if not all the sectors required was cached (this isn't an error condition)
//    1 if all of the sectors required was cached, and the destination buffer was filled up with the required data
//
unsigned int ui_journaling_cache_read(unsigned char drive, char *buff, unsigned int ui_sector, unsigned char uc_num_of_sector_to_read);

// check if the journaling cache can contain the writing blocks
//
// input:
//   drive: the drive where write to
//   buff : the source buffer where to read the data to cache
//   ui_sector: the start sector write number
//   uc_num_of_sector_to_write: the number of sector to write
//
// returns:
//    0 if not all the sectors required was cached, this is an error condition!
//    1 if all of the sectors required was cached, and the destination buffer was copied into the journal cache
//
unsigned int ui_journaling_cache_write(unsigned char drive, char *buff, unsigned int ui_sector, unsigned char uc_num_of_sector_to_write);

#endif

#endif //#ifndef def_az4186_journaling_h_included
