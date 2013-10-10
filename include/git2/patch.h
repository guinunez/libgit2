/*
 * Copyright (C) the libgit2 contributors. All rights reserved.
 *
 * This file is part of libgit2, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */
#ifndef INCLUDE_git_patch_h__
#define INCLUDE_git_patch_h__

#include "common.h"
#include "types.h"
#include "oid.h"

/**
 * Structure describing a hunk of a diff.
 */
typedef struct {
	size_t num_lines;  /**< total lines of data in this hunk */
	size_t old_start;  /**< Starting line number in old file */
	size_t old_lines;  /**< Number of lines in old file */
	size_t new_start;  /** Starting line number in new file */
	size_t new_lines;  /**< Number of lines in new file */
	size_t header_len; /**< String length of hunk header text */
	char   header[GIT_FLEX_ARRAY]; /**< hunk header text */
} git_patch_hunk;

/**
 * Line origin constants.
 *
 * These values describe where a line came from and will be passed to
 * the git_patch_data_cb when iterating over a diff.  There are some
 * special origin constants at the end that are used for the text
 * output callbacks to demarcate lines that are actually part of
 * the file or hunk headers.
 */
typedef enum {
	/* These values will be sent to `git_patch_data_cb` along with the line */
	GIT_PATCH_LINE_CONTEXT   = ' ',
	GIT_PATCH_LINE_ADDITION  = '+',
	GIT_PATCH_LINE_DELETION  = '-',

	GIT_PATCH_LINE_CONTEXT_EOFNL = '=', /**< Both files have no LF at end */
	GIT_PATCH_LINE_ADD_EOFNL = '>',     /**< Old has no LF at end, new does */
	GIT_PATCH_LINE_DEL_EOFNL = '<',     /**< Old has LF at end, new does not */

	/* The following values will only be sent to a `git_patch_data_cb` when
	 * the content of a diff is being formatted (eg. through
	 * git_patch_print_patch() or git_patch_print_compact(), for instance).
	 */
	GIT_PATCH_LINE_FILE_HDR  = 'F',
	GIT_PATCH_LINE_HUNK_HDR  = 'H',
	GIT_PATCH_LINE_BINARY    = 'B' /**< For "Binary files x and y differ" */
} git_patch_line_t;

/**
 * Structure describing a span of diff data
 *
 * This will generally be a line of diff data, but for word diffs, there
 * may be multiple entries like this for a single line.
 */
typedef struct {
	git_patch_line_t line_origin;
	const char *content;
	size_t content_len;
	size_t old_lineno;
	size_t new_lineno;
} git_patch_line;

/**
 * Return the patch for an entry in the diff list.
 *
 * The `git_patch` is a newly created object contains the text diffs for
 * the delta.  You have to call `git_patch_free()` when you are done with
 * it.  You can use the patch object to loop over all the hunks and lines
 * in the diff of the one delta.
 *
 * For an unchanged file or a binary file, no `git_patch` will be created,
 * the output will be set to NULL, and the `binary` flag will be set true
 * in the `git_delta` structure.
 *
 * @param out Output parameter for the delta patch object
 * @param diff The diff to read delta from
 * @param delta_index The index of delta to format
 * @return 0 on success, other value < 0 on error
 */
GIT_EXTERN(int) git_patch_from_diff_delta(
	git_patch **out,
	const git_diff *diff,
	size_t delta_index,
	const git_diff_format_options *format_opts);

/**
 * Directly generate a patch from the difference between two blobs.
 *
 * This generates a patch object for the difference.  You can use the
 * standard `git_patch` accessor functions to read the patch data, and you
 * must call `git_patch_free()` on the patch when done.
 *
 * @param out The generated patch; NULL on error
 * @param old_blob Blob for old side of diff, or NULL for empty blob
 * @param old_as_path Treat old blob as if it had this filename; can be NULL
 * @param new_blob Blob for new side of diff, or NULL for empty blob
 * @param new_as_path Treat new blob as if it had this filename; can be NULL
 * @param opts Options for diff, or NULL for default options
 * @return 0 on success or error code < 0
 */
GIT_EXTERN(int) git_patch_from_blobs(
	git_patch **out,
	const git_blob *old_blob,
	const char *old_as_path,
	const git_blob *new_blob,
	const char *new_as_path,
	const git_diff_options *diff_opts,
	const git_diff_format_options *format_opts);

/**
 * Directly generate a patch from the difference between a blob and a buffer.
 *
 * This is just like `git_diff_blobs()` except it generates a patch object
 * for the difference instead of directly making callbacks.  You can use
 * the standard `git_patch` accessor functions to read the patch data, and
 * you must call `git_patch_free()` on the patch when done.
 *
 * @param out The generated patch; NULL on error
 * @param old_blob Blob for old side of diff, or NULL for empty blob
 * @param old_as_path Treat old blob as if it had this filename; can be NULL
 * @param buffer Raw data for new side of diff, or NULL for empty
 * @param buffer_len Length of raw data for new side of diff
 * @param buffer_as_path Treat buffer as if it had this filename; can be NULL
 * @param opts Options for diff, or NULL for default options
 * @return 0 on success or error code < 0
 */
GIT_EXTERN(int) git_patch_from_blob_and_buffer(
	git_patch **out,
	const git_blob *old_blob,
	const char *old_as_path,
	const char *buffer,
	size_t buffer_len,
	const char *buffer_as_path,
	const git_diff_options *diff_opts,
	const git_diff_format_options *format_opts);

/**
 * Free a git_patch object.
 */
GIT_EXTERN(void) git_patch_free(git_patch *patch);

/**
 * Get the delta associated with a patch
 */
GIT_EXTERN(const git_delta *) git_patch_delta(git_patch *patch);

/**
 * Get the number of hunks in a patch
 */
GIT_EXTERN(size_t) git_patch_num_hunks(git_patch *patch);

/**
 * Get line counts of each type in a patch.
 *
 * This helps imitate a diff --numstat type of output.  For that purpose,
 * you only need the `total_additions` and `total_deletions` values, but we
 * include the `total_context` line count in case you want the total number
 * of lines of diff output that will be generated.
 *
 * All outputs are optional. Pass NULL if you don't need a particular count.
 *
 * @param total_context Count of context lines in output, can be NULL.
 * @param total_additions Count of addition lines in output, can be NULL.
 * @param total_deletions Count of deletion lines in output, can be NULL.
 * @param patch The git_patch object
 * @return 0 on success, <0 on error
 */
GIT_EXTERN(int) git_patch_line_stats(
	size_t *total_context,
	size_t *total_additions,
	size_t *total_deletions,
	const git_patch *patch);

/**
 * Get the information about a hunk in a patch
 *
 * Given a patch and a hunk index into the patch, this returns detailed
 * information about that hunk.  Any of the output pointers can be passed
 * as NULL if you don't care about that particular piece of information.
 *
 * @param hunk Output pointer to git_patch_hunk structure
 * @param patch Input pointer to patch object
 * @param hunk_idx Input index of hunk to get information about
 * @return 0 on success, GIT_ENOTFOUND if hunk_idx out of range, <0 on error
 */
GIT_EXTERN(int) git_patch_get_hunk(
	const git_patch_hunk **hunk,
	const git_patch *patch,
	size_t hunk_idx);

/**
 * Get the number of lines in a hunk.
 *
 * @param patch The git_patch object
 * @param hunk_idx Index of the hunk
 * @return Number of lines in hunk or -1 if invalid hunk index
 */
GIT_EXTERN(int) git_patch_num_lines_in_hunk(
	const git_patch *patch,
	size_t hunk_idx);

/**
 * Get data about a line in a hunk of a patch.
 *
 * Given a patch, a hunk index, and a line index in the hunk, this
 * will return a lot of details about that line.  If you pass a hunk
 * index larger than the number of hunks or a line index larger than
 * the number of lines in the hunk, this will return -1.
 *
 * @param line_origin A GIT_DIFF_LINE constant from above
 * @param content Pointer to content of diff line, not NUL-terminated
 * @param content_len Number of characters in content
 * @param old_lineno Line number in old file or -1 if line is added
 * @param new_lineno Line number in new file or -1 if line is deleted
 * @param patch The patch to look in
 * @param hunk_idx The index of the hunk
 * @param line_of_hunk The index of the line in the hunk
 * @return 0 on success, <0 on failure
 */
GIT_EXTERN(int) git_patch_get_line_in_hunk(
	const git_patch_line **line,
	const git_patch *patch,
	size_t hunk_idx,
	size_t line_of_hunk);

/**
 * Look up size of patch diff data in bytes
 *
 * This returns the raw size of the patch data.  This only includes the
 * actual data from the lines of the diff, not the file or hunk headers.
 *
 * If you pass `include_context` as true (non-zero), this will be the size
 * of all of the diff output; if you pass it as false (zero), this will
 * only include the actual changed lines (as if `context_lines` was 0).
 *
 * @param patch A git_patch representing changes to one file
 * @param include_context Include context lines in size if non-zero
 * @param include_hunk_headers Include hunk header lines if non-zero
 * @param include_file_headers Include file header lines if non-zero
 * @return The number of bytes of data
 */
GIT_EXTERN(size_t) git_patch_size(
	const git_patch *patch,
	int include_context,
	int include_hunk_headers,
	int include_file_headers);

/**
 * Get the content of a patch as a single diff text.
 *
 * This will honor the GIT_DIFF_FORMAT value from the options structure
 * with the exception that the default will print the whole patch, not
 * just the header.
 *
 * @param output Buffer to write output into
 * @param patch A git_patch representing changes to one file
 * @return 0 on success, <0 on failure.
 */
GIT_EXTERN(int) git_patch_to_str(
	git_buf *output,
	const git_patch *patch);

#endif
