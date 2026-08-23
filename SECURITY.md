# Security policy

dcmmlib moves user-selected files to Trash. Bugs in path allowlists can destroy data.

## Please report privately

**Do not** open a public issue for:

- Ways to trash `/`, `/System`, home, Documents, keychains, Mail, Photos, `.ssh`, or similar
- Bypass of `isSafeToTrash` / protected prefixes
- Path traversal, symlink escapes, or copy-then-unlink fallbacks

Use [GitHub Security Advisories](https://github.com/DHANUSH-web/dcmmlib/security/advisories/new) on this repository.

## What is in scope

- Trash / Recycle Bin handling
- Catalog paths that include secrets or user documents
- C ABI buffer handling
- Tests that could run against a real home directory

## What is out of scope

- “I selected the file and it went to Trash” (intended)
- Empty Trash after the user confirmed (intended permanent delete)

We will credit reporters who follow this process unless they ask otherwise.
