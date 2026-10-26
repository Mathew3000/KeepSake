# Backup payload format and recovery

The HTTPS protocol transfers **opaque encrypted bytes**. `<sha256>.bundle`
means *immutable object*; it does not imply a custom archive or filesystem format.

## Recommended initial payload
Use the synchronized KeePass `.kdbx` file as the input to `server/publish.py`.
The published `.bundle` file contains exactly the original KDBX bytes (not ZIP
and not additionally encrypted). In the emergency recovery procedure, copy a
verified snapshot to a safe computer and **rename the copy** to
`KeePass-Recovery.kdbx`; open it with KeePassXC or KeePass. Never edit the
source file on the SD card.

Documentation can be stored as **KeePass attachments** (which are inside the
encrypted database), or in a separately encrypted companion artifact with
explicit, documented recovery instructions. Do not concatenate documents to
the KDBX: that would make it unreadable. Never publish sensitive Markdown or
HTML documentation directly as plaintext to the endpoint.

## Restorability checks
1. Read back the SD bytes and verify SHA-256 against the published manifest.
2. Make a copy of the selected `.bundle` file, rename it to `.kdbx`, and open
   with the documented KeePass/KeePassXC release.
3. Test the separately sealed master password, optional key file, and attachments.
4. Test from a clean computer without requiring your own servers.
