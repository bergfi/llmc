; ModuleID = 'common_processed_nolines.c'
source_filename = "common_processed_nolines.c"

%_IO_FILE_plus = type {}
%_IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %_IO_marker*, %_IO_FILE*, i32, i32, i32, i16, i8, [1 x i8], i8*, i32, i8*, i8*, i8*, i8*, i64, i32, [0 x i8] }
%_IO_marker = type { %_IO_marker*, %_IO_FILE*, i32 }
%poptOption = type { i8*, i8, i32, i8*, i32, i8*, i8* }
%transition_info = type { i32*, i32, i32 }
%unnamed_struct_1 = type { [2 x i32] }
%unnamed_struct_3 = type { i32, %unnamed_union_4 }
%unnamed_union_4 = type { i32 }
%unnamed_struct_4 = type { i32, %unnamed_struct_3 }
%unnamed_struct_5 = type { i32, %unnamed_struct_3 }
%poptItem_s = type { %poptOption, i32, i8** }
%poptContext_s = type {}
%poptBits_s = type { [1 x i32] }
%__locale_struct = type { [13 x %__locale_data*], i16*, i32*, i32*, [13 x i8*] }
%__locale_data = type {}
%unnamed_struct_16 = type { i32, i8* }
%unnamed_struct_17 = type { i32, [0 x i8] }
%value_table_s = type {}
%table_iterator_s = type {}
%matrix_table_struct = type {}
%table_factory_s = type {}
%unnamed_struct_22 = type { i32, i32 }
%unnamed_struct_23 = type { i64, i64 }
%unnamed_struct_24 = type { i64, i64 }
%unnamed_struct_25 = type { [0 x i64] }
%unnamed_struct_28 = type { [0 x i64] }
%pthread_attr_t = type { [56 x i8] }
%__pthread_internal_list = type { %__pthread_internal_list*, %__pthread_internal_list* }
%unnamed_union_30 = type { %__pthread_mutex_s }
%__pthread_mutex_s = type { i32, i32, i32, i32, i32, i16, i16, %__pthread_internal_list }
%unnamed_union_31 = type { [4 x i8] }
%unnamed_union_31.0 = type { %unnamed_struct_31 }
%unnamed_struct_31 = type { i32, i32, i64, i64, i64, i8*, i32, i32 }
%unnamed_union_32 = type { [4 x i8] }
%unnamed_union_32.1 = type { [56 x i8] }
%unnamed_union_33 = type { [8 x i8] }
%unnamed_union_33.2 = type { [32 x i8] }
%unnamed_union_33.3 = type { [4 x i8] }
%bitvector = type { i64, i64, i64* }
%unnamed_struct_36 = type { i32, i32, i32 }
%unnamed_struct_37 = type { i32, %unnamed_struct_36*, i32*, i32*, i32* }
%unnamed_struct_38 = type { i32, i32, i32, i32* }
%matrix = type { i32, i32, i32, %unnamed_struct_37, %unnamed_struct_37, %bitvector }
%dm_row_iterator = type { %matrix*, i32, i32 }
%dm_col_iterator = type { %matrix*, i32, i32 }
%lts_type_s = type {}
%string_string_map = type {}
%string_set = type {}
%grey_box_model = type {}
%sl_group = type { i32, [0 x i32] }
%guard = type { i32, [0 x i32] }
%poptAlias = type { i8*, i8, i32, i8** }
%timeval = type { i64, i64 }
%timespec = type { i64, i64 }
%random_data = type { i32*, i32*, i32*, i32, i32, i32, i32* }
%drand48_data = type { [3 x i16], [3 x i16], i16, i16, i64 }

@_IO_2_1_stdin_ = external global %_IO_FILE_plus
@_IO_2_1_stdout_ = external global %_IO_FILE_plus
@_IO_2_1_stderr_ = external global %_IO_FILE_plus
@stdin = external global %_IO_FILE*
@stdout = external global %_IO_FILE*
@stderr = external global %_IO_FILE*
@sys_nerr = external global i32
@sys_errlist = external global [0 x i8*]
@poptAliasOptions = external global [0 x %poptOption]
@poptHelpOptions = external global [0 x %poptOption]
@poptHelpOptionsI18N = external global %poptOption*
@_poptBitsN = external global i32
@_poptBitsM = external global i32
@_poptBitsK = external global i32
@__environ = external global i8**
@optarg = external global i8*
@optind = external global i32
@opterr = external global i32
@optopt = external global i32
@GB_NO_TRANSITION = external global %transition_info
@greybox_options = external global [0 x %poptOption]

@size_t = alias i64, i64* null
@__u_char = alias i8, i8* null
@__u_short = alias i16, i16* null
@__u_int = alias i32, i32* null
@__u_long = alias i64, i64* null
@__int8_t = alias i8, i8* null
@__uint8_t = alias i8, i8* null
@__int16_t = alias i16, i16* null
@__uint16_t = alias i16, i16* null
@__int32_t = alias i32, i32* null
@__uint32_t = alias i32, i32* null
@__int64_t = alias i64, i64* null
@__uint64_t = alias i64, i64* null
@__u_quad_t = alias i64, i64* null
@__dev_t = alias i64, i64* null
@__uid_t = alias i32, i32* null
@__gid_t = alias i32, i32* null
@__ino_t = alias i64, i64* null
@__ino64_t = alias i64, i64* null
@__mode_t = alias i32, i32* null
@__nlink_t = alias i64, i64* null
@__pid_t = alias i32, i32* null
@__fsid_t = alias %unnamed_struct_1, %unnamed_struct_1* null
@__clock_t = alias i64, i64* null
@__rlim_t = alias i64, i64* null
@__rlim64_t = alias i64, i64* null
@__id_t = alias i32, i32* null
@__time_t = alias i64, i64* null
@__useconds_t = alias i32, i32* null
@__suseconds_t = alias i64, i64* null
@__daddr_t = alias i32, i32* null
@__key_t = alias i32, i32* null
@__clockid_t = alias i32, i32* null
@__timer_t = alias i8*, i8** null
@__blksize_t = alias i64, i64* null
@__blkcnt_t = alias i64, i64* null
@__blkcnt64_t = alias i64, i64* null
@__fsblkcnt_t = alias i64, i64* null
@__fsblkcnt64_t = alias i64, i64* null
@__fsfilcnt_t = alias i64, i64* null
@__fsfilcnt64_t = alias i64, i64* null
@__fsword_t = alias i64, i64* null
@__ssize_t = alias i64, i64* null
@__syscall_slong_t = alias i64, i64* null
@__syscall_ulong_t = alias i64, i64* null
@__loff_t = alias i32, i32* null
@__qaddr_t = alias i32*, i32** null
@__caddr_t = alias i8*, i8** null
@__intptr_t = alias i64, i64* null
@__socklen_t = alias i32, i32* null
@FILE = alias %_IO_FILE, %_IO_FILE* null
@__FILE = alias %_IO_FILE, %_IO_FILE* null
@__mbstate_t = alias %unnamed_struct_3, %unnamed_struct_3* null
@_G_fpos_t = alias %unnamed_struct_4, %unnamed_struct_4* null
@_G_fpos64_t = alias %unnamed_struct_5, %unnamed_struct_5* null
@__gnuc_va_list = alias i32, i32* null
@_IO_FILE = alias %_IO_FILE, %_IO_FILE* null
@__io_read_fn = alias i64 (i8*, i8*, i64), i64 (i8*, i8*, i64)* null
@__io_write_fn = alias i64 (i8*, i8*, i64), i64 (i8*, i8*, i64)* null
@__io_seek_fn = alias i32 (i8*, i32*, i32), i32 (i8*, i32*, i32)* null
@__io_close_fn = alias i32 (i8*), i32 (i8*)* null
@va_list = alias i32, i32* null
@off_t = alias i32, i32* null
@ssize_t = alias i64, i64* null
@fpos_t = alias %unnamed_struct_4, %unnamed_struct_4* null
@poptItem = alias %poptItem_s*, %poptItem_s** null
@poptContext = alias %poptContext_s*, %poptContext_s** null
@poptOption = alias %poptOption*, %poptOption** null
@poptCallbackType = alias void (%poptContext_s*, i32, %poptOption*, i8*, i8*)*, void (%poptContext_s*, i32, %poptOption*, i8*, i8*)** null
@poptBits = alias %poptBits_s*, %poptBits_s** null
@int_least8_t = alias i8, i8* null
@int_least16_t = alias i16, i16* null
@int_least32_t = alias i32, i32* null
@int_least64_t = alias i64, i64* null
@uint_least8_t = alias i8, i8* null
@uint_least16_t = alias i16, i16* null
@uint_least32_t = alias i32, i32* null
@uint_least64_t = alias i64, i64* null
@int_fast8_t = alias i8, i8* null
@int_fast16_t = alias i64, i64* null
@int_fast32_t = alias i64, i64* null
@int_fast64_t = alias i64, i64* null
@uint_fast8_t = alias i8, i8* null
@uint_fast16_t = alias i64, i64* null
@uint_fast32_t = alias i64, i64* null
@uint_fast64_t = alias i64, i64* null
@intptr_t = alias i64, i64* null
@uintptr_t = alias i64, i64* null
@__locale_t = alias %__locale_struct*, %__locale_struct** null
@locale_t = alias %__locale_struct*, %__locale_struct** null
@chunk_len = alias i32, i32* null
@chunk = alias %unnamed_struct_16, %unnamed_struct_16* null
@pchunk = alias %unnamed_struct_17, %unnamed_struct_17* null
@gid_t = alias i32, i32* null
@uid_t = alias i32, i32* null
@useconds_t = alias i32, i32* null
@pid_t = alias i32, i32* null
@socklen_t = alias i32, i32* null
@value_index_t = alias i32, i32* null
@value_table_t = alias %value_table_s*, %value_table_s** null
@value_table_create_t = alias %value_table_s* (i8*, i8*)*, %value_table_s* (i8*, i8*)** null
@user_destroy_t = alias void (%value_table_s*)*, void (%value_table_s*)** null
@put_native_t = alias i32 (%value_table_s*, i32)*, i32 (%value_table_s*, i32)** null
@get_native_t = alias void (%value_table_s*, i32, i32)*, void (%value_table_s*, i32, i32)** null
@put_chunk_t = alias i32 (%value_table_s*, %unnamed_struct_16)*, i32 (%value_table_s*, %unnamed_struct_16)** null
@put_at_chunk_t = alias void (%value_table_s*, %unnamed_struct_16, i32)*, void (%value_table_s*, %unnamed_struct_16, i32)** null
@get_chunk_t = alias %unnamed_struct_16 (%value_table_s*, i32)*, %unnamed_struct_16 (%value_table_s*, i32)** null
@vt_get_count_t = alias i32 (%value_table_s*)*, i32 (%value_table_s*)** null
@table_iterator_t = alias %table_iterator_s*, %table_iterator_s** null
@vt_iterator_t = alias %table_iterator_s* (%value_table_s*)*, %table_iterator_s* (%value_table_s*)** null
@it_next_t = alias %unnamed_struct_16 (%table_iterator_s*)*, %unnamed_struct_16 (%table_iterator_s*)** null
@it_has_next_t = alias i32 (%table_iterator_s*)*, i32 (%table_iterator_s*)** null
@matrix_table_t = alias %matrix_table_struct*, %matrix_table_struct** null
@table_factory_t = alias %table_factory_s*, %table_factory_s** null
@tf_new_map_t = alias %value_table_s* (%table_factory_s*)*, %value_table_s* (%table_factory_s*)** null
@wchar_t = alias i32, i32* null
@div_t = alias %unnamed_struct_22, %unnamed_struct_22* null
@ldiv_t = alias %unnamed_struct_23, %unnamed_struct_23* null
@lldiv_t = alias %unnamed_struct_24, %unnamed_struct_24* null
@u_char = alias i8, i8* null
@u_short = alias i16, i16* null
@u_int = alias i32, i32* null
@u_long = alias i64, i64* null
@quad_t = alias i32, i32* null
@u_quad_t = alias i64, i64* null
@fsid_t = alias %unnamed_struct_1, %unnamed_struct_1* null
@loff_t = alias i32, i32* null
@ino_t = alias i64, i64* null
@dev_t = alias i64, i64* null
@mode_t = alias i32, i32* null
@nlink_t = alias i64, i64* null
@id_t = alias i32, i32* null
@daddr_t = alias i32, i32* null
@caddr_t = alias i8*, i8** null
@key_t = alias i32, i32* null
@clock_t = alias i64, i64* null
@time_t = alias i64, i64* null
@clockid_t = alias i32, i32* null
@timer_t = alias i8*, i8** null
@ulong = alias i64, i64* null
@ushort = alias i16, i16* null
@uint = alias i32, i32* null
@u_int8_t = alias i32, i32* null
@u_int16_t = alias i32, i32* null
@u_int32_t = alias i32, i32* null
@u_int64_t = alias i32, i32* null
@register_t = alias i32, i32* null
@__sig_atomic_t = alias i32, i32* null
@__sigset_t = alias %unnamed_struct_25, %unnamed_struct_25* null
@sigset_t = alias %unnamed_struct_25, %unnamed_struct_25* null
@suseconds_t = alias i64, i64* null
@__fd_mask = alias i64, i64* null
@fd_set = alias %unnamed_struct_28, %unnamed_struct_28* null
@fd_mask = alias i64, i64* null
@blksize_t = alias i64, i64* null
@blkcnt_t = alias i64, i64* null
@fsblkcnt_t = alias i64, i64* null
@fsfilcnt_t = alias i64, i64* null
@pthread_t = alias i64, i64* null
@pthread_attr_t = alias %pthread_attr_t, %pthread_attr_t* null
@__pthread_list_t = alias %__pthread_internal_list, %__pthread_internal_list* null
@pthread_mutex_t = alias %unnamed_union_30, %unnamed_union_30* null
@pthread_mutexattr_t = alias %unnamed_union_31, %unnamed_union_31* null
@pthread_cond_t = alias %unnamed_union_31.0, %unnamed_union_31.0* null
@pthread_condattr_t = alias %unnamed_union_32, %unnamed_union_32* null
@pthread_key_t = alias i32, i32* null
@pthread_once_t = alias i32, i32* null
@pthread_rwlock_t = alias %unnamed_union_32.1, %unnamed_union_32.1* null
@pthread_rwlockattr_t = alias %unnamed_union_33, %unnamed_union_33* null
@pthread_spinlock_t = alias i32, i32* null
@pthread_barrier_t = alias %unnamed_union_33.2, %unnamed_union_33.2* null
@pthread_barrierattr_t = alias %unnamed_union_33.3, %unnamed_union_33.3* null
@__compar_fn_t = alias i32 (i8*, i8*)*, i32 (i8*, i8*)** null
@bitvector_t = alias %bitvector, %bitvector* null
@header_entry_t = alias %unnamed_struct_36, %unnamed_struct_36* null
@matrix_header_t = alias %unnamed_struct_37, %unnamed_struct_37* null
@permutation_group_t = alias %unnamed_struct_38, %unnamed_struct_38* null
@matrix_t = alias %matrix, %matrix* null
@dm_comparator_fn = alias i32 (%matrix*, i32, i32)*, i32 (%matrix*, i32, i32)** null
@dm_subsume_rows_fn = alias i32 (%matrix*, i32, i32, i8*)*, i32 (%matrix*, i32, i32, i8*)** null
@dm_nub_rows_fn = alias i32 (%matrix*, i32, i32, i8*)*, i32 (%matrix*, i32, i32, i8*)** null
@dm_subsume_cols_fn = alias i32 (%matrix*, i32, i32, i8*)*, i32 (%matrix*, i32, i32, i8*)** null
@dm_nub_cols_fn = alias i32 (%matrix*, i32, i32, i8*)*, i32 (%matrix*, i32, i32, i8*)** null
@dm_permute_fn = alias void (%matrix*, %unnamed_struct_38*)*, void (%matrix*, %unnamed_struct_38*)** null
@dm_cost_t = alias i32, i32* null
@dm_row_iterator_t = alias %dm_row_iterator, %dm_row_iterator* null
@dm_col_iterator_t = alias %dm_col_iterator, %dm_col_iterator* null
@lts_type_t = alias %lts_type_s*, %lts_type_s** null
@data_format_t = alias i32, i32* null
@string_map_t = alias %string_string_map*, %string_string_map** null
@string_set_t = alias %string_set*, %string_set** null
@index_class_t = alias i32, i32* null
@pins_strictness_t = alias i32, i32* null
@model_t = alias %grey_box_model*, %grey_box_model** null
@transition_info_t = alias %transition_info, %transition_info* null
@sl_group_enum_t = alias i32, i32* null
@sl_group_t = alias %sl_group, %sl_group* null
@guard_t = alias %guard, %guard* null
@TransitionCB = alias void (i8*, %transition_info*, i32*, i32*)*, void (i8*, %transition_info*, i32*, i32*)** null
@covered_by_grey_t = alias i32 (i32*, i32*)*, i32 (i32*, i32*)** null
@pins_loader_t = alias void (%grey_box_model*, i8*)*, void (%grey_box_model*, i8*)** null
@next_method_grey_t = alias i32 (%grey_box_model*, i32, i32*, void (i8*, %transition_info*, i32*, i32*)*, i8*)*, i32 (%grey_box_model*, i32, i32*, void (i8*, %transition_info*, i32*, i32*)*, i8*)** null
@next_method_matching_t = alias i32 (%grey_box_model*, i32, i32, i32*, void (i8*, %transition_info*, i32*, i32*)*, i8*)*, i32 (%grey_box_model*, i32, i32, i32*, void (i8*, %transition_info*, i32*, i32*)*, i8*)** null
@next_method_black_t = alias i32 (%grey_box_model*, i32*, void (i8*, %transition_info*, i32*, i32*)*, i8*)*, i32 (%grey_box_model*, i32*, void (i8*, %transition_info*, i32*, i32*)*, i8*)** null
@get_label_all_method_t = alias void (%grey_box_model*, i32*, i32*)*, void (%grey_box_model*, i32*, i32*)** null
@get_label_group_method_t = alias void (%grey_box_model*, i32, i32*, i32*)*, void (%grey_box_model*, i32, i32*, i32*)** null
@get_label_method_t = alias i32 (%grey_box_model*, i32, i32*)*, i32 (%grey_box_model*, i32, i32*)** null
@groups_of_edge_t = alias i32 (%grey_box_model*, i32, i32, i32**)*, i32 (%grey_box_model*, i32, i32, i32**)** null
@chunk2pretty_t = alias i32 (%grey_box_model*, i32, i32)*, i32 (%grey_box_model*, i32, i32)** null
@ExitCB = alias void (%grey_box_model*)*, void (%grey_box_model*)** null

declare i32 @__underflow(%_IO_FILE*)

declare i32 @__uflow(%_IO_FILE*)

declare i32 @__overflow(%_IO_FILE*, i32)

declare i32 @_IO_getc(%_IO_FILE*)

declare i32 @_IO_putc(i32, %_IO_FILE*)

declare i32 @_IO_feof(%_IO_FILE*)

declare i32 @_IO_ferror(%_IO_FILE*)

declare i32 @_IO_peekc_locked(%_IO_FILE*)

declare void @_IO_flockfile(%_IO_FILE*)

declare void @_IO_funlockfile(%_IO_FILE*)

declare i32 @_IO_ftrylockfile(%_IO_FILE*)

declare i32 @_IO_vfscanf(%_IO_FILE*, i8*, i32, i32*)

declare i32 @_IO_vfprintf(%_IO_FILE*, i8*, i32)

declare i64 @_IO_padn(%_IO_FILE*, i32, i64)

declare i64 @_IO_sgetn(%_IO_FILE*, i8*, i64)

declare i32 @_IO_seekoff(%_IO_FILE*, i32, i32, i32)

declare i32 @_IO_seekpos(%_IO_FILE*, i32, i32)

declare void @_IO_free_backup_area(%_IO_FILE*)

declare i32 @remove(i8*)

declare i32 @rename(i8*, i8*)

declare i32 @renameat(i32, i8*, i32, i8*)

declare %_IO_FILE* @tmpfile()

declare i8* @tmpnam(i8*)

declare i8* @tmpnam_r(i8*)

declare i8* @tempnam(i8*, i8*)

declare i32 @fclose(%_IO_FILE*)

declare i32 @fflush(%_IO_FILE*)

declare i32 @fflush_unlocked(%_IO_FILE*)

declare %_IO_FILE* @fopen(i8*, i8*)

declare %_IO_FILE* @freopen(i8*, i8*, %_IO_FILE*)

declare %_IO_FILE* @fdopen(i32, i8*)

declare %_IO_FILE* @fmemopen(i8*, i64, i8*)

declare %_IO_FILE* @open_memstream(i8**, i64*)

declare void @setbuf(%_IO_FILE*, i8*)

declare i32 @setvbuf(%_IO_FILE*, i8*, i32, i64)

declare void @setbuffer(%_IO_FILE*, i8*, i64)

declare void @setlinebuf(%_IO_FILE*)

declare i32 @fprintf(%_IO_FILE*, i8*, ...)

declare i32 @printf(i8*, ...)

declare i32 @sprintf(i8*, i8*, ...)

declare i32 @vfprintf(%_IO_FILE*, i8*, i32)

declare i32 @vprintf(i8*, i32)

declare i32 @vsprintf(i8*, i8*, i32)

declare i32 @snprintf(i8*, i64, i8*, ...)

declare i32 @vsnprintf(i8*, i64, i8*, i32)

declare i32 @vdprintf(i32, i8*, i32)

declare i32 @dprintf(i32, i8*, ...)

declare i32 @fscanf(%_IO_FILE*, i8*, ...)

declare i32 @scanf(i8*, ...)

declare i32 @sscanf(i8*, i8*, ...)

declare i32 @vfscanf(%_IO_FILE*, i8*, i32)

declare i32 @vscanf(i8*, i32)

declare i32 @vsscanf(i8*, i8*, i32)

declare i32 @fgetc(%_IO_FILE*)

declare i32 @getc(%_IO_FILE*)

declare i32 @getchar()

declare i32 @getc_unlocked(%_IO_FILE*)

declare i32 @getchar_unlocked()

declare i32 @fgetc_unlocked(%_IO_FILE*)

declare i32 @fputc(i32, %_IO_FILE*)

declare i32 @putc(i32, %_IO_FILE*)

declare i32 @putchar(i32)

declare i32 @fputc_unlocked(i32, %_IO_FILE*)

declare i32 @putc_unlocked(i32, %_IO_FILE*)

declare i32 @putchar_unlocked(i32)

declare i32 @getw(%_IO_FILE*)

declare i32 @putw(i32, %_IO_FILE*)

declare i8* @fgets(i8*, i32, %_IO_FILE*)

declare i64 @__getdelim(i8**, i64*, i32, %_IO_FILE*)

declare i64 @getdelim(i8**, i64*, i32, %_IO_FILE*)

declare i64 @getline(i8**, i64*, %_IO_FILE*)

declare i32 @fputs(i8*, %_IO_FILE*)

declare i32 @puts(i8*)

declare i32 @ungetc(i32, %_IO_FILE*)

declare i64 @fread(i8*, i64, i64, %_IO_FILE*)

declare i64 @fwrite(i8*, i64, i64, %_IO_FILE*)

declare i64 @fread_unlocked(i8*, i64, i64, %_IO_FILE*)

declare i64 @fwrite_unlocked(i8*, i64, i64, %_IO_FILE*)

declare i32 @fseek(%_IO_FILE*, i64, i32)

declare i64 @ftell(%_IO_FILE*)

declare void @rewind(%_IO_FILE*)

declare i32 @fseeko(%_IO_FILE*, i32, i32)

declare i32 @ftello(%_IO_FILE*)

declare i32 @fgetpos(%_IO_FILE*, %unnamed_struct_4*)

declare i32 @fsetpos(%_IO_FILE*, %unnamed_struct_4*)

declare void @clearerr(%_IO_FILE*)

declare i32 @feof(%_IO_FILE*)

declare i32 @ferror(%_IO_FILE*)

declare void @clearerr_unlocked(%_IO_FILE*)

declare i32 @feof_unlocked(%_IO_FILE*)

declare i32 @ferror_unlocked(%_IO_FILE*)

declare void @perror(i8*)

declare i32 @fileno(%_IO_FILE*)

declare i32 @fileno_unlocked(%_IO_FILE*)

declare %_IO_FILE* @popen(i8*, i8*)

declare i32 @pclose(%_IO_FILE*)

declare i8* @ctermid(i8*)

declare void @flockfile(%_IO_FILE*)

declare i32 @ftrylockfile(%_IO_FILE*)

declare void @funlockfile(%_IO_FILE*)

declare %poptContext_s* @poptFreeContext(%poptContext_s*)

declare %poptContext_s* @poptGetContext(i8*, i32, i8**, %poptOption*, i32)

declare %poptContext_s* @poptFini(%poptContext_s*)

declare %poptContext_s* @poptInit(i32, i8**, %poptOption*, i8*)

declare void @poptResetContext(%poptContext_s*)

declare i32 @poptGetNextOpt(%poptContext_s*)

declare i8* @poptGetOptArg(%poptContext_s*)

declare i8* @poptGetArg(%poptContext_s*)

declare i8* @poptPeekArg(%poptContext_s*)

declare i8** @poptGetArgs(%poptContext_s*)

declare i8* @poptBadOption(%poptContext_s*, i32)

declare i32 @poptStuffArgs(%poptContext_s*, i8**)

declare i32 @poptAddAlias(%poptContext_s*, %poptAlias, i32)

declare i32 @poptAddItem(%poptContext_s*, %poptItem_s*, i32)

declare i32 @poptSaneFile(i8*)

declare i32 @poptReadFile(i8*, i8**, i64*, i32)

declare i32 @poptReadConfigFile(%poptContext_s*, i8*)

declare i32 @poptReadConfigFiles(%poptContext_s*, i8*)

declare i32 @poptReadDefaultConfig(%poptContext_s*, i32)

declare i32 @poptDupArgv(i32, i8**, i32*, i8***)

declare i32 @poptParseArgvString(i8*, i32*, i8***)

declare i32 @poptConfigFileToString(%_IO_FILE*, i8**, i32)

declare i8* @poptStrerror(i32)

declare void @poptSetExecPath(%poptContext_s*, i8*, i32)

declare void @poptPrintHelp(%poptContext_s*, %_IO_FILE*, i32)

declare void @poptPrintUsage(%poptContext_s*, %_IO_FILE*, i32)

declare void @poptSetOtherOptionHelp(%poptContext_s*, i8*)

declare i8* @poptGetInvocationName(%poptContext_s*)

declare i32 @poptStrippedArgv(%poptContext_s*, i32, i8**)

declare i32 @poptSaveString(i8***, i32, i8*)

declare i32 @poptSaveLongLong(i64*, i32, i64)

declare i32 @poptSaveLong(i64*, i32, i64)

declare i32 @poptSaveShort(i16*, i32, i64)

declare i32 @poptSaveInt(i32*, i32, i64)

declare i32 @poptBitsAdd(%poptBits_s*, i8*)

declare i32 @poptBitsChk(%poptBits_s*, i8*)

declare i32 @poptBitsClr(%poptBits_s*)

declare i32 @poptBitsDel(%poptBits_s*, i8*)

declare i32 @poptBitsIntersect(%poptBits_s**, %poptBits_s*)

declare i32 @poptBitsUnion(%poptBits_s**, %poptBits_s*)

declare i32 @poptBitsArgs(%poptContext_s*, %poptBits_s**)

declare i32 @poptSaveBits(%poptBits_s**, i32, i8*)

declare i8* @memcpy(i8*, i8*, i64)

declare i8* @memmove(i8*, i8*, i64)

declare i8* @memccpy(i8*, i8*, i32, i64)

declare i8* @memset(i8*, i32, i64)

declare i32 @memcmp(i8*, i8*, i64)

declare i8* @memchr(i8*, i32, i64)

declare i8* @strcpy(i8*, i8*)

declare i8* @strncpy(i8*, i8*, i64)

declare i8* @strcat(i8*, i8*)

declare i8* @strncat(i8*, i8*, i64)

declare i32 @strcmp(i8*, i8*)

declare i32 @strncmp(i8*, i8*, i64)

declare i32 @strcoll(i8*, i8*)

declare i64 @strxfrm(i8*, i8*, i64)

declare i32 @strcoll_l(i8*, i8*, %__locale_struct*)

declare i64 @strxfrm_l(i8*, i8*, i64, %__locale_struct*)

declare i8* @strdup(i8*)

declare i8* @strndup(i8*, i64)

declare i8* @strchr(i8*, i32)

declare i8* @strrchr(i8*, i32)

declare i64 @strcspn(i8*, i8*)

declare i64 @strspn(i8*, i8*)

declare i8* @strpbrk(i8*, i8*)

declare i8* @strstr(i8*, i8*)

declare i8* @strtok(i8*, i8*)

declare i8* @__strtok_r(i8*, i8*, i8**)

declare i8* @strtok_r(i8*, i8*, i8**)

declare i64 @strlen(i8*)

declare i64 @strnlen(i8*, i64)

declare i8* @strerror(i32)

declare i32 @strerror_r(i32, i8*, i64)

declare i8* @strerror_l(i32, %__locale_struct*)

declare void @__bzero(i8*, i64)

declare void @bcopy(i8*, i8*, i64)

declare void @bzero(i8*, i64)

declare i32 @bcmp(i8*, i8*, i64)

declare i8* @index(i8*, i32)

declare i8* @rindex(i8*, i32)

declare i32 @ffs(i32)

declare i32 @strcasecmp(i8*, i8*)

declare i32 @strncasecmp(i8*, i8*, i64)

declare i8* @strsep(i8**, i8*)

declare i8* @strsignal(i32)

declare i8* @__stpcpy(i8*, i8*)

declare i8* @stpcpy(i8*, i8*)

declare i8* @__stpncpy(i8*, i8*, i64)

declare i8* @stpncpy(i8*, i8*, i64)

declare void @chunk_encode_copy(%unnamed_struct_16, %unnamed_struct_16, i8)

declare void @chunk_decode_copy(%unnamed_struct_16, %unnamed_struct_16, i8)

declare void @chunk2string(%unnamed_struct_16, i64, i8*)

declare void @string2chunk(i8*, %unnamed_struct_16*)

declare i32 @access(i8*, i32)

declare i32 @faccessat(i32, i8*, i32, i32)

declare i32 @lseek(i32, i32, i32)

declare i32 @close(i32)

declare i64 @read(i32, i8*, i64)

declare i64 @write(i32, i8*, i64)

declare i64 @pread(i32, i8*, i64, i32)

declare i64 @pwrite(i32, i8*, i64, i32)

declare i32 @pipe([2 x i32])

declare i32 @alarm(i32)

declare i32 @sleep(i32)

declare i32 @ualarm(i32, i32)

declare i32 @usleep(i32)

declare i32 @pause()

declare i32 @chown(i8*, i32, i32)

declare i32 @fchown(i32, i32, i32)

declare i32 @lchown(i8*, i32, i32)

declare i32 @fchownat(i32, i8*, i32, i32, i32)

declare i32 @chdir(i8*)

declare i32 @fchdir(i32)

declare i8* @getcwd(i8*, i64)

declare i8* @getwd(i8*)

declare i32 @dup(i32)

declare i32 @dup2(i32, i32)

declare i32 @execve(i8*, [0 x i8*], [0 x i8*])

declare i32 @fexecve(i32, [0 x i8*], [0 x i8*])

declare i32 @execv(i8*, [0 x i8*])

declare i32 @execle(i8*, i8*, ...)

declare i32 @execl(i8*, i8*, ...)

declare i32 @execvp(i8*, [0 x i8*])

declare i32 @execlp(i8*, i8*, ...)

declare i32 @nice(i32)

declare void @_exit(i32)

declare i64 @pathconf(i8*, i32)

declare i64 @fpathconf(i32, i32)

declare i64 @sysconf(i32)

declare i64 @confstr(i32, i8*, i64)

declare i32 @getpid()

declare i32 @getppid()

declare i32 @getpgrp()

declare i32 @__getpgid(i32)

declare i32 @getpgid(i32)

declare i32 @setpgid(i32, i32)

declare i32 @setpgrp()

declare i32 @setsid()

declare i32 @getsid(i32)

declare i32 @getuid()

declare i32 @geteuid()

declare i32 @getgid()

declare i32 @getegid()

declare i32 @getgroups(i32, [0 x i32])

declare i32 @setuid(i32)

declare i32 @setreuid(i32, i32)

declare i32 @seteuid(i32)

declare i32 @setgid(i32)

declare i32 @setregid(i32, i32)

declare i32 @setegid(i32)

declare i32 @fork()

declare i32 @vfork()

declare i8* @ttyname(i32)

declare i32 @ttyname_r(i32, i8*, i64)

declare i32 @isatty(i32)

declare i32 @ttyslot()

declare i32 @link(i8*, i8*)

declare i32 @linkat(i32, i8*, i32, i8*, i32)

declare i32 @symlink(i8*, i8*)

declare i64 @readlink(i8*, i8*, i64)

declare i32 @symlinkat(i8*, i32, i8*)

declare i64 @readlinkat(i32, i8*, i8*, i64)

declare i32 @unlink(i8*)

declare i32 @unlinkat(i32, i8*, i32)

declare i32 @rmdir(i8*)

declare i32 @tcgetpgrp(i32)

declare i32 @tcsetpgrp(i32, i32)

declare i8* @getlogin()

declare i32 @getlogin_r(i8*, i64)

declare i32 @setlogin(i8*)

declare i32 @getopt(i32, i8**, i8*)

declare i32 @gethostname(i8*, i64)

declare i32 @sethostname(i8*, i64)

declare i32 @sethostid(i64)

declare i32 @getdomainname(i8*, i64)

declare i32 @setdomainname(i8*, i64)

declare i32 @vhangup()

declare i32 @revoke(i8*)

declare i32 @profil(i16*, i64, i64, i32)

declare i32 @acct(i8*)

declare i8* @getusershell()

declare void @endusershell()

declare void @setusershell()

declare i32 @daemon(i32, i32)

declare i32 @chroot(i8*)

declare i8* @getpass(i8*)

declare i32 @fsync(i32)

declare i64 @gethostid()

declare void @sync()

declare i32 @getpagesize()

declare i32 @getdtablesize()

declare i32 @truncate(i8*, i32)

declare i32 @ftruncate(i32, i32)

declare i32 @brk(i8*)

declare i8* @sbrk(i64)

declare i64 @syscall(i64, ...)

declare i32 @lockf(i32, i32, i32)

declare i32 @fdatasync(i32)

declare %value_table_s* @VTcreateBase(i8*, i64)

declare void @VTrealloc(%value_table_s*, i64)

declare void @VTdestroy(%value_table_s*)

declare void @VTdestroyZ(%value_table_s**)

declare void @VTdestroySet(%value_table_s*, void (%value_table_s*)*)

declare i8* @VTgetType(%value_table_s*)

declare i32 @VTputNative(%value_table_s*, ...)

declare void @VTputNativeSet(%value_table_s*, i32 (%value_table_s*, i32)*)

declare void @VTgetNative(%value_table_s*, i32, ...)

declare void @VTgetNativeSet(%value_table_s*, void (%value_table_s*, i32, i32)*)

declare i32 @VTputChunk(%value_table_s*, %unnamed_struct_16)

declare void @VTputChunkSet(%value_table_s*, i32 (%value_table_s*, %unnamed_struct_16)*)

declare void @VTputAtChunk(%value_table_s*, %unnamed_struct_16, i32)

declare void @VTputAtChunkSet(%value_table_s*, void (%value_table_s*, %unnamed_struct_16, i32)*)

declare %unnamed_struct_16 @VTgetChunk(%value_table_s*, i32)

declare void @VTgetChunkSet(%value_table_s*, %unnamed_struct_16 (%value_table_s*, i32)*)

declare i32 @VTgetCount(%value_table_s*)

declare void @VTgetCountSet(%value_table_s*, i32 (%value_table_s*)*)

declare %table_iterator_s* @VTiterator(%value_table_s*)

declare void @VTiteratorSet(%value_table_s*, %table_iterator_s* (%value_table_s*)*)

declare %table_iterator_s* @ITcreateBase(i64)

declare %unnamed_struct_16 @ITnext(%table_iterator_s*)

declare void @ITnextSet(%table_iterator_s*, %unnamed_struct_16 (%table_iterator_s*)*)

declare i32 @IThasNext(%table_iterator_s*)

declare void @IThasNextSet(%table_iterator_s*, i32 (%table_iterator_s*)*)

declare %matrix_table_struct* @MTcreate(i32)

declare void @MTdestroy(%matrix_table_struct*)

declare void @MTdestroyZ(%matrix_table_struct**)

declare i32 @MTgetWidth(%matrix_table_struct*)

declare i32 @MTgetCount(%matrix_table_struct*)

declare void @MTaddRow(%matrix_table_struct*, i32*)

declare void @MTgetRow(%matrix_table_struct*, i32, i32*)

declare void @MTupdate(%matrix_table_struct*, i32, i32, i32)

declare void @MTclusterBuild(%matrix_table_struct*, i32, i32)

declare void @MTclusterSort(%matrix_table_struct*, i32)

declare i32 @MTclusterSize(%matrix_table_struct*, i32)

declare i32* @MTclusterMapBegin(%matrix_table_struct*)

declare i32* @MTclusterMapColumn(%matrix_table_struct*, i32)

declare void @MTclusterGetRow(%matrix_table_struct*, i32, i32, i32*)

declare i32 @MTclusterGetElem(%matrix_table_struct*, i32, i32, i32)

declare i32 @MTclusterCount(%matrix_table_struct*)

declare void @MTclusterUpdate(%matrix_table_struct*, i32, i32, i32, i32)

declare i32 @MTgetMax(%matrix_table_struct*, i32)

declare void @MTsimplify(%matrix_table_struct*, %matrix_table_struct*)

declare %value_table_s* @TFnewTable(%table_factory_s*)

declare void @TFnewTableSet(%table_factory_s*, %value_table_s* (%table_factory_s*)*)

declare %table_factory_s* @TFcreateBase(i64)

declare %value_table_s* @simple_chunk_table_create(i8*, i8*)

declare %table_factory_s* @simple_table_factory_create()

declare %table_iterator_s* @simple_iterator_create(%value_table_s*)

declare i64 @__ctype_get_mb_cur_max()

declare double @atof(i8*)

declare i32 @atoi(i8*)

declare i64 @atol(i8*)

declare i64 @atoll(i8*)

declare double @strtod(i8*, i8**)

declare float @strtof(i8*, i8**)

declare x86_fp80 @strtold(i8*, i8**)

declare i64 @strtol(i8*, i8**, i32)

declare i64 @strtoul(i8*, i8**, i32)

declare i64 @strtoq(i8*, i8**, i32)

declare i64 @strtouq(i8*, i8**, i32)

declare i64 @strtoll(i8*, i8**, i32)

declare i64 @strtoull(i8*, i8**, i32)

declare i8* @l64a(i64)

declare i64 @a64l(i8*)

declare i32 @__bswap_32(i32)

declare i64 @__bswap_64(i64)

declare i32 @select(i32, %unnamed_struct_28*, %unnamed_struct_28*, %unnamed_struct_28*, %timeval*)

declare i32 @pselect(i32, %unnamed_struct_28*, %unnamed_struct_28*, %unnamed_struct_28*, %timespec*, %unnamed_struct_25*)

declare i32 @gnu_dev_major(i64)

declare i32 @gnu_dev_minor(i64)

declare i64 @gnu_dev_makedev(i32, i32)

declare i64 @random()

declare void @srandom(i32)

declare i8* @initstate(i32, i8*, i64)

declare i8* @setstate(i8*)

declare i32 @random_r(%random_data*, i32*)

declare i32 @srandom_r(i32, %random_data*)

declare i32 @initstate_r(i32, i8*, i64, %random_data*)

declare i32 @setstate_r(i8*, %random_data*)

declare i32 @rand()

declare void @srand(i32)

declare i32 @rand_r(i32*)

declare double @drand48()

declare double @erand48([3 x i16])

declare i64 @lrand48()

declare i64 @nrand48([3 x i16])

declare i64 @mrand48()

declare i64 @jrand48([3 x i16])

declare void @srand48(i64)

declare i16* @seed48([3 x i16])

declare void @lcong48([7 x i16])

declare i32 @drand48_r(%drand48_data*, double*)

declare i32 @erand48_r([3 x i16], %drand48_data*, double*)

declare i32 @lrand48_r(%drand48_data*, i64*)

declare i32 @nrand48_r([3 x i16], %drand48_data*, i64*)

declare i32 @mrand48_r(%drand48_data*, i64*)

declare i32 @jrand48_r([3 x i16], %drand48_data*, i64*)

declare i32 @srand48_r(i64, %drand48_data*)

declare i32 @seed48_r([3 x i16], %drand48_data*)

declare i32 @lcong48_r([7 x i16], %drand48_data*)

declare i8* @malloc(i64)

declare i8* @calloc(i64, i64)

declare i8* @realloc(i8*, i64)

declare void @free(i8*)

declare void @cfree(i8*)

declare i8* @alloca(i64)

declare i8* @valloc(i64)

declare i32 @posix_memalign(i8**, i64, i64)

declare i8* @aligned_alloc(i64, i64)

declare void @abort()

declare i32 @atexit(void ()*)

declare i32 @at_quick_exit(void ()*)

declare i32 @on_exit(void (i32, i8*)*, i8*)

declare void @exit(i32)

declare void @quick_exit(i32)

declare void @_Exit(i32)

declare i8* @getenv(i8*)

declare i32 @putenv(i8*)

declare i32 @setenv(i8*, i8*, i32)

declare i32 @unsetenv(i8*)

declare i32 @clearenv()

declare i8* @mktemp(i8*)

declare i32 @mkstemp(i8*)

declare i32 @mkstemps(i8*, i32)

declare i8* @mkdtemp(i8*)

declare i32 @system(i8*)

declare i8* @realpath(i8*, i8*)

declare i8* @bsearch(i8*, i8*, i64, i64, i32 (i8*, i8*)*)

declare void @qsort(i8*, i64, i64, i32 (i8*, i8*)*)

declare i32 @abs(i32)

declare i64 @labs(i64)

declare i64 @llabs(i64)

declare %unnamed_struct_22 @div(i32, i32)

declare %unnamed_struct_23 @ldiv(i64, i64)

declare %unnamed_struct_24 @lldiv(i64, i64)

declare i8* @ecvt(double, i32, i32*, i32*)

declare i8* @fcvt(double, i32, i32*, i32*)

declare i8* @gcvt(double, i32, i8*)

declare i8* @qecvt(x86_fp80, i32, i32*, i32*)

declare i8* @qfcvt(x86_fp80, i32, i32*, i32*)

declare i8* @qgcvt(x86_fp80, i32, i8*)

declare i32 @ecvt_r(double, i32, i32*, i32*, i8*, i64)

declare i32 @fcvt_r(double, i32, i32*, i32*, i8*, i64)

declare i32 @qecvt_r(x86_fp80, i32, i32*, i32*, i8*, i64)

declare i32 @qfcvt_r(x86_fp80, i32, i32*, i32*, i8*, i64)

declare i32 @mblen(i8*, i64)

declare i32 @mbtowc(i32*, i8*, i64)

declare i32 @wctomb(i8*, i32)

declare i64 @mbstowcs(i32*, i8*, i64)

declare i64 @wcstombs(i8*, i32*, i64)

declare i32 @rpmatch(i8*)

declare i32 @getsubopt(i8**, i8**, i8**)

declare i32 @getloadavg([0 x double], i32)

declare void @bitvector_create(%bitvector*, i64)

declare void @bitvector_clear(%bitvector*)

declare void @bitvector_free(%bitvector*)

declare void @bitvector_copy(%bitvector*, %bitvector*)

declare i64 @bitvector_size(%bitvector*)

declare i32 @bitvector_isset_or_set(%bitvector*, i64)

declare void @bitvector_set2(%bitvector*, i64, i64)

declare i32 @bitvector_isset_or_set2(%bitvector*, i64, i64)

declare i32 @bitvector_get2(%bitvector*, i64)

declare void @bitvector_set(%bitvector*, i64)

declare void @bitvector_set_atomic(%bitvector*, i64)

declare void @bitvector_unset(%bitvector*, i64)

declare i32 @bitvector_is_set(%bitvector*, i64)

declare void @bitvector_union(%bitvector*, %bitvector*)

declare void @bitvector_intersect(%bitvector*, %bitvector*)

declare i32 @bitvector_is_empty(%bitvector*)

declare i32 @bitvector_is_disjoint(%bitvector*, %bitvector*)

declare void @bitvector_invert(%bitvector*)

declare i64 @bitvector_n_high(%bitvector*)

declare void @bitvector_high_bits(%bitvector*, i32*)

declare i32 @bitvector_equal(%bitvector*, %bitvector*)

declare void @bitvector_xor(%bitvector*, %bitvector*)

declare void @dm_create_header(%unnamed_struct_37*, i32)

declare void @dm_free_header(%unnamed_struct_37*)

declare void @dm_create_permutation_group(%unnamed_struct_38*, i32, i32*)

declare void @dm_free_permutation_group(%unnamed_struct_38*)

declare void @dm_add_to_permutation_group(%unnamed_struct_38*, i32)

declare void @dm_close_group(%unnamed_struct_38*)

declare void @dm_create_permutation_groups(%unnamed_struct_38**, i32*, i32*, i32)

declare void @dm_copy_row_info(%matrix*, %matrix*)

declare void @dm_copy_col_info(%matrix*, %matrix*)

declare void @dm_create(%matrix*, i32, i32)

declare void @dm_free(%matrix*)

declare i32 @dm_nrows(%matrix*)

declare i32 @dm_ncols(%matrix*)

declare void @dm_set(%matrix*, i32, i32)

declare void @dm_unset(%matrix*, i32, i32)

declare i32 @dm_is_set(%matrix*, i32, i32)

declare void @dm_permute_rows(%matrix*, %unnamed_struct_38*)

declare void @dm_permute_cols(%matrix*, %unnamed_struct_38*)

declare void @dm_swap_rows(%matrix*, i32, i32)

declare void @dm_swap_cols(%matrix*, i32, i32)

declare void @dm_copy(%matrix*, %matrix*)

declare void @dm_make_sparse(%matrix*)

declare void @dm_flatten(%matrix*)

declare void @dm_sort_rows(%matrix*, i32 (%matrix*, i32, i32)*)

declare void @dm_sort_cols(%matrix*, i32 (%matrix*, i32, i32)*)

declare void @dm_nub_rows(%matrix*, i32 (%matrix*, i32, i32, i8*)*, i8*)

declare void @dm_nub_cols(%matrix*, i32 (%matrix*, i32, i32, i8*)*, i8*)

declare void @dm_subsume_rows(%matrix*, i32 (%matrix*, i32, i32, i8*)*, i8*)

declare void @dm_subsume_cols(%matrix*, i32 (%matrix*, i32, i32, i8*)*, i8*)

declare void @dm_ungroup_rows(%matrix*)

declare void @dm_ungroup_cols(%matrix*)

declare void @dm_print(%_IO_FILE*, %matrix*)

declare void @dm_print_combined(%_IO_FILE*, %matrix*, %matrix*, %matrix*)

declare void @dm_anneal(%matrix*, i32, i32)

declare void @dm_optimize(%matrix*)

declare void @dm_all_perm(%matrix*)

declare void @dm_FORCE(%matrix*)

declare void @dm_horizontal_flip(%matrix*)

declare void @dm_vertical_flip(%matrix*)

declare i32 @dm_first(%matrix*, i32)

declare i32 @dm_last(%matrix*, i32)

declare i32 @dm_top(%matrix*, i32)

declare i32 @dm_bottom(%matrix*, i32)

declare i32** @dm_rows_to_idx_table(%matrix*)

declare i32** @dm_cols_to_idx_table(%matrix*)

declare void @dm_create_col_iterator(%dm_col_iterator*, %matrix*, i32)

declare void @dm_create_row_iterator(%dm_row_iterator*, %matrix*, i32)

declare i32 @dm_col_next(%dm_col_iterator*)

declare i32 @dm_row_next(%dm_row_iterator*)

declare i32 @dm_ones_in_row(%matrix*, i32)

declare i32 @dm_ones_in_col(%matrix*, i32)

declare i32 @dm_project_vector(%matrix*, i32, i32*, i32*)

declare i32 @dm_expand_vector(%matrix*, i32, i32*, i32*, i32*)

declare void @dm_print_perm(%unnamed_struct_37*)

declare void @dm_clear(%matrix*)

declare void @dm_fill(%matrix*)

declare void @dm_apply_or(%matrix*, %matrix*)

declare i32 @dm_equals(%matrix*, %matrix*)

declare void @dm_apply_xor(%matrix*, %matrix*)

declare i32 @dm_is_empty(%matrix*)

declare void @dm_prod(%bitvector*, %bitvector*, %matrix*)

declare void @dm_row_union(%bitvector*, %matrix*, i32)

declare void @dm_col_union(%bitvector*, %matrix*, i32)

declare void @dm_row_spans(%matrix*, i32*)

declare double @dm_event_span(%matrix*, i32*)

declare double @dm_weighted_event_span(%matrix*, i32*)

declare i32 @dm_equal_header(%unnamed_struct_37*, %unnamed_struct_37*)

declare i8* @data_format_string(%lts_type_s*, i32)

declare i8* @data_format_string_generic(i32)

declare %lts_type_s* @lts_type_create()

declare %lts_type_s* @lts_type_clone(%lts_type_s*)

declare %lts_type_s* @lts_type_permute(%lts_type_s*, i32*)

declare void @lts_type_destroy(%lts_type_s**)

declare void @lts_type_printf(i8*, %lts_type_s*)

declare void @lts_type_set_state_length(%lts_type_s*, i32)

declare i32 @lts_type_get_state_length(%lts_type_s*)

declare void @lts_type_set_state_name(%lts_type_s*, i32, i8*)

declare i8* @lts_type_get_state_name(%lts_type_s*, i32)

declare void @lts_type_set_state_type(%lts_type_s*, i32, i8*)

declare i8* @lts_type_get_state_type(%lts_type_s*, i32)

declare void @lts_type_set_state_typeno(%lts_type_s*, i32, i32)

declare i32 @lts_type_get_state_typeno(%lts_type_s*, i32)

declare void @lts_type_set_state_label_count(%lts_type_s*, i32)

declare i32 @lts_type_get_state_label_count(%lts_type_s*)

declare void @lts_type_set_state_label_name(%lts_type_s*, i32, i8*)

declare void @lts_type_set_state_label_type(%lts_type_s*, i32, i8*)

declare void @lts_type_set_state_label_typeno(%lts_type_s*, i32, i32)

declare i8* @lts_type_get_state_label_name(%lts_type_s*, i32)

declare i8* @lts_type_get_state_label_type(%lts_type_s*, i32)

declare i32 @lts_type_get_state_label_typeno(%lts_type_s*, i32)

declare i32 @lts_type_find_state_label_prefix(%lts_type_s*, i8*)

declare i32 @lts_type_find_state_label(%lts_type_s*, i8*)

declare void @lts_type_set_edge_label_count(%lts_type_s*, i32)

declare i32 @lts_type_get_edge_label_count(%lts_type_s*)

declare void @lts_type_set_edge_label_name(%lts_type_s*, i32, i8*)

declare void @lts_type_set_edge_label_type(%lts_type_s*, i32, i8*)

declare void @lts_type_set_edge_label_typeno(%lts_type_s*, i32, i32)

declare i8* @lts_type_get_edge_label_name(%lts_type_s*, i32)

declare i8* @lts_type_get_edge_label_type(%lts_type_s*, i32)

declare i32 @lts_type_get_edge_label_typeno(%lts_type_s*, i32)

declare i32 @lts_type_find_edge_label_prefix(%lts_type_s*, i8*)

declare i32 @lts_type_find_edge_label(%lts_type_s*, i8*)

declare i32 @lts_type_get_type_count(%lts_type_s*)

declare i32 @lts_type_add_type(%lts_type_s*, i8*, i32*)

declare i8* @lts_type_get_type(%lts_type_s*, i32)

declare i32 @lts_type_has_type(%lts_type_s*, i8*)

declare i32 @lts_type_get_format(%lts_type_s*, i32)

declare void @lts_type_set_format(%lts_type_s*, i32, i32)

declare i32 @lts_type_get_max(%lts_type_s*, i32)

declare i32 @lts_type_get_min(%lts_type_s*, i32)

declare void @lts_type_set_range(%lts_type_s*, i32, i32, i32)

declare i32 @lts_type_put_type(%lts_type_s*, i8*, i32, i32*)

declare i32 @lts_type_find_type_prefix(%lts_type_s*, i8*)

declare i32 @lts_type_find_type(%lts_type_s*, i8*)

declare void @lts_type_validate(%lts_type_s*)

declare %string_string_map* @SSMcreateSWP(i8*)

declare i8* @SSMcall(%string_string_map*, i8*)

declare %string_set* @SSMcreateSWPset(i8*)

declare i32 @SSMmember(%string_set*, i8*)

declare i32 @GBsetMatrix(%grey_box_model*, i8*, %matrix*, i32, i32, i32)

declare i32 @GBgetMatrixID(%grey_box_model*, i8*)

declare i32 @GBgetMatrixCount(%grey_box_model*)

declare %matrix* @GBgetMatrix(%grey_box_model*, i32)

declare i8* @GBgetMatrixName(%grey_box_model*, i32)

declare i32 @GBgetMatrixStrictness(%grey_box_model*, i32)

declare i32 @GBgetMatrixRowInfo(%grey_box_model*, i32)

declare i32 @GBgetMatrixColumnInfo(%grey_box_model*, i32)

declare void @GBloadFile(%grey_box_model*, i8*, %grey_box_model**)

declare void @GBloadFileShared(%grey_box_model*, i8*)

declare %lts_type_s* @GBgetLTStype(%grey_box_model*)

declare void @GBprintDependencyMatrix(%_IO_FILE*, %grey_box_model*)

declare void @GBprintDependencyMatrixRead(%_IO_FILE*, %grey_box_model*)

declare void @GBprintDependencyMatrixMayWrite(%_IO_FILE*, %grey_box_model*)

declare void @GBprintDependencyMatrixMustWrite(%_IO_FILE*, %grey_box_model*)

declare void @GBprintDependencyMatrixCombined(%_IO_FILE*, %grey_box_model*)

declare void @GBprintStateLabelMatrix(%_IO_FILE*, %grey_box_model*)

declare void @GBprintStateLabelGroupInfo(%_IO_FILE*, %grey_box_model*)

declare %matrix* @GBgetDMInfo(%grey_box_model*)

declare %matrix* @GBgetDMInfoRead(%grey_box_model*)

declare %matrix* @GBgetDMInfoMayWrite(%grey_box_model*)

declare %matrix* @GBgetDMInfoMustWrite(%grey_box_model*)

declare void @GBsetDMInfo(%grey_box_model*, %matrix*)

declare void @GBsetDMInfoRead(%grey_box_model*, %matrix*)

declare void @GBsetDMInfoMayWrite(%grey_box_model*, %matrix*)

declare void @GBsetDMInfoMustWrite(%grey_box_model*, %matrix*)

declare void @GBgetInitialState(%grey_box_model*, i32*)

declare i32 @GBgetTransitionsShort(%grey_box_model*, i32, i32*, void (i8*, %transition_info*, i32*, i32*)*, i8*)

declare i32 @GBgetTransitionsShortR2W(%grey_box_model*, i32, i32*, void (i8*, %transition_info*, i32*, i32*)*, i8*)

declare i32 @GBgetTransitionsLong(%grey_box_model*, i32, i32*, void (i8*, %transition_info*, i32*, i32*)*, i8*)

declare i32 @GBgetActionsShort(%grey_box_model*, i32, i32*, void (i8*, %transition_info*, i32*, i32*)*, i8*)

declare i32 @GBgetActionsShortR2W(%grey_box_model*, i32, i32*, void (i8*, %transition_info*, i32*, i32*)*, i8*)

declare i32 @GBgetActionsLong(%grey_box_model*, i32, i32*, void (i8*, %transition_info*, i32*, i32*)*, i8*)

declare i32 @GBgetTransitionsMarked(%grey_box_model*, %matrix*, i32, i32*, void (i8*, %transition_info*, i32*, i32*)*, i8*)

declare i32 @GBgetTransitionsMatching(%grey_box_model*, i32, i32, i32*, void (i8*, %transition_info*, i32*, i32*)*, i8*)

declare i32 @GBgetTransitionsAll(%grey_box_model*, i32*, void (i8*, %transition_info*, i32*, i32*)*, i8*)

declare void @GBsetIsCoveredBy(%grey_box_model*, i32 (i32*, i32*)*)

declare void @GBsetIsCoveredByShort(%grey_box_model*, i32 (i32*, i32*)*)

declare i32 @GBisCoveredByShort(%grey_box_model*, i32*, i32*)

declare i32 @GBisCoveredBy(%grey_box_model*, i32*, i32*)

declare %matrix* @GBgetStateLabelInfo(%grey_box_model*)

declare i32 @GBgetStateLabelShort(%grey_box_model*, i32, i32*)

declare i32 @GBgetStateLabelLong(%grey_box_model*, i32, i32*)

declare void @GBgetStateLabelsGroup(%grey_box_model*, i32, i32*, i32*)

declare void @GBgetStateLabelsAll(%grey_box_model*, i32*, i32*)

declare %sl_group* @GBgetStateLabelGroupInfo(%grey_box_model*, i32)

declare void @GBsetStateLabelGroupInfo(%grey_box_model*, i32, %sl_group*)

declare i32 @GBgetStateAll(%grey_box_model*, i32*, i32*, void (i8*, %transition_info*, i32*, i32*)*, i8*)

declare i32 @GBgroupsOfEdge(%grey_box_model*, i32, i32, i32**)

declare %string_set* @GBgetDefaultFilter(%grey_box_model*)

declare %grey_box_model* @GBcreateBase()

declare %grey_box_model* @GBgetParent(%grey_box_model*)

declare void @GBregisterLoader(i8*, void (%grey_box_model*, i8*)*)

declare void @GBregisterPreLoader(i8*, void (%grey_box_model*, i8*)*)

declare void @GBsetContext(%grey_box_model*, i8*)

declare i8* @GBgetContext(%grey_box_model*)

declare void @GBsetLTStype(%grey_box_model*, %lts_type_s*)

declare void @GBsetStateLabelInfo(%grey_box_model*, %matrix*)

declare i32 @GBhasGuardsInfo(%grey_box_model*)

declare void @GBsetGuardsInfo(%grey_box_model*, %guard**)

declare %guard** @GBgetGuardsInfo(%grey_box_model*)

declare void @GBsetGuard(%grey_box_model*, i32, %guard*)

declare void @GBsetGuardCoEnabledInfo(%grey_box_model*, %matrix*)

declare void @GBsetDoNotAccordInfo(%grey_box_model*, %matrix*)

declare %matrix* @GBgetDoNotAccordInfo(%grey_box_model*)

declare void @GBsetCommutesInfo(%grey_box_model*, %matrix*)

declare %matrix* @GBgetCommutesInfo(%grey_box_model*)

declare %matrix* @GBgetGuardCoEnabledInfo(%grey_box_model*)

declare %guard* @GBgetGuard(%grey_box_model*, i32)

declare void @GBsetGuardNESInfo(%grey_box_model*, %matrix*)

declare %matrix* @GBgetGuardNESInfo(%grey_box_model*)

declare void @GBsetGuardNDSInfo(%grey_box_model*, %matrix*)

declare %matrix* @GBgetGuardNDSInfo(%grey_box_model*)

declare void @GBsetPorGroupVisibility(%grey_box_model*, i32*)

declare i32* @GBgetPorGroupVisibility(%grey_box_model*)

declare void @GBsetPorStateLabelVisibility(%grey_box_model*, i32*)

declare i32* @GBgetPorStateLabelVisibility(%grey_box_model*)

declare void @GBsetInitialState(%grey_box_model*, i32*)

declare void @GBsetNextStateLong(%grey_box_model*, i32 (%grey_box_model*, i32, i32*, void (i8*, %transition_info*, i32*, i32*)*, i8*)*)

declare void @GBsetNextStateShort(%grey_box_model*, i32 (%grey_box_model*, i32, i32*, void (i8*, %transition_info*, i32*, i32*)*, i8*)*)

declare void @GBsetNextStateShortR2W(%grey_box_model*, i32 (%grey_box_model*, i32, i32*, void (i8*, %transition_info*, i32*, i32*)*, i8*)*)

declare void @GBsetActionsLong(%grey_box_model*, i32 (%grey_box_model*, i32, i32*, void (i8*, %transition_info*, i32*, i32*)*, i8*)*)

declare void @GBsetActionsShort(%grey_box_model*, i32 (%grey_box_model*, i32, i32*, void (i8*, %transition_info*, i32*, i32*)*, i8*)*)

declare void @GBsetActionsShortR2W(%grey_box_model*, i32 (%grey_box_model*, i32, i32*, void (i8*, %transition_info*, i32*, i32*)*, i8*)*)

declare void @GBsetNextStateMatching(%grey_box_model*, i32 (%grey_box_model*, i32, i32, i32*, void (i8*, %transition_info*, i32*, i32*)*, i8*)*)

declare void @GBsetNextStateAll(%grey_box_model*, i32 (%grey_box_model*, i32*, void (i8*, %transition_info*, i32*, i32*)*, i8*)*)

declare void @GBsetStateLabelsAll(%grey_box_model*, void (%grey_box_model*, i32*, i32*)*)

declare void @GBsetStateLabelsGroup(%grey_box_model*, void (%grey_box_model*, i32, i32*, i32*)*)

declare void @GBsetStateLabelLong(%grey_box_model*, i32 (%grey_box_model*, i32, i32*)*)

declare void @GBsetStateLabelShort(%grey_box_model*, i32 (%grey_box_model*, i32, i32*)*)

declare void @GBsetGroupsOfEdge(%grey_box_model*, i32 (%grey_box_model*, i32, i32, i32**)*)

declare void @GBsetDefaultFilter(%grey_box_model*, %string_set*)

declare void @GBsetChunkMap(%grey_box_model*, %table_factory_s*)

declare void @GBcopyChunkMaps(%grey_box_model*, %grey_box_model*)

declare void @GBgrowChunkMaps(%grey_box_model*, i32)

declare void @GBinitModelDefaults(%grey_box_model**, %grey_box_model*)

declare i32 @GBchunkPrettyPrint(%grey_box_model*, i32, i32)

declare void @GBsetPrettyPrint(%grey_box_model*, i32 (%grey_box_model*, i32, i32)*)

declare %value_table_s* @GBgetChunkMap(%grey_box_model*, i32)

declare void @GBsetExit(%grey_box_model*, void (%grey_box_model*)*)

declare void @GBExit(%grey_box_model*)

declare void @GBsetVarPerm(%grey_box_model*, i32*)

declare i32* @GBgetVarPerm(%grey_box_model*)

declare void @GBsetGroupPerm(%grey_box_model*, i32*)

declare i32* @GBgetGroupPerm(%grey_box_model*)

declare i64 @pins_get_state_label_count(%grey_box_model*)

declare i64 @pins_get_edge_label_count(%grey_box_model*)

declare i64 @pins_get_state_variable_count(%grey_box_model*)

declare i64 @pins_get_group_count(%grey_box_model*)

declare void @pins_add_edge_label_visible(%grey_box_model*, i32, i32)

declare void @pins_add_group_visible(%grey_box_model*, i32)

declare void @pins_add_state_variable_visible(%grey_box_model*, i32)

declare void @pins_add_state_label_visible(%grey_box_model*, i32)

declare i32 @pins_get_accepting_set_edge_label_index(%grey_box_model*)

declare i32 @pins_get_accepting_state_label_index(%grey_box_model*)

declare i32 @pins_get_progress_state_label_index(%grey_box_model*)

declare i32 @pins_get_weak_ltl_progress_state_label_index(%grey_box_model*)

declare i32 @pins_get_valid_end_state_label_index(%grey_box_model*)

declare i32 @pins_state_is_accepting(%grey_box_model*, i32*)

declare i32 @pins_state_is_progress(%grey_box_model*, i32*)

declare i32 @pins_state_is_weak_ltl_progress(%grey_box_model*, i32*)

declare i32 @pins_state_is_valid_end(%grey_box_model*, i32*)

declare %table_iterator_s* @pins_chunk_iterator(%grey_box_model*, i32)

declare i32 @pins_chunk_count(%grey_box_model*, i32)

declare void @pins_chunk_put_at(%grey_box_model*, i32, %unnamed_struct_16, i32)

declare i32 @pins_chunk_put(%grey_box_model*, i32, %unnamed_struct_16)

declare %unnamed_struct_16 @pins_chunk_get(%grey_box_model*, i32, i32)

declare i64 @pins_chunk_put64(%grey_box_model*, i32, %unnamed_struct_16)

declare i64 @pins_chunk_cam64(%grey_box_model*, i32, i64, i32, i32*, i32)

declare i64 @pins_chunk_append64(%grey_box_model*, i32, i64, i32*, i32)

declare i64 @pins_chunk_getpartial64(%grey_box_model*, i32, i64, i32, i32*, i32)

declare %unnamed_struct_16 @pins_chunk_get64(%grey_box_model*, i32, i64)

declare i64 @dmc_state_cam(i8*, i32, i32*, i32, %transition_info*)
