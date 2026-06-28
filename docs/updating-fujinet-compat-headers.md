# Updating the Compat Layer When fujinet-lib Changes Upstream

`libs/fujinet-compat-amiga/include/fujinet-network.h` and `fujinet-fuji.h` are
verbatim copies of the upstream headers from `FujiNetWIFI/fujinet-lib`. When
that repo changes, update the compat layer as follows.

## 1. Re-download the headers

```bash
gh api repos/FujiNetWIFI/fujinet-lib/contents/fujinet-network.h \
    --jq '.content' | base64 -d \
    > libs/fujinet-compat-amiga/include/fujinet-network.h

gh api repos/FujiNetWIFI/fujinet-lib/contents/fujinet-fuji.h \
    --jq '.content' | base64 -d \
    > libs/fujinet-compat-amiga/include/fujinet-fuji.h
```

## 2. Review the diff

```bash
git diff libs/fujinet-compat-amiga/include/
```

This shows exactly what the upstream added, removed, or changed.

## 3. Build — let the compiler find the gaps

```bash
cd libs/fujinet-compat-amiga && make
```

New functions or types without implementations will cause compile or link
errors. That is the completeness check.

## 4. For each new symbol

Categorize it using the same scheme as `docs/audit-track1a-gap-table.md`:

- **Direct map** — add a thin wrapper in `src/fn_network.c` or `src/fn_fuji.c`
- **Needs work** — implement; may require a new `fn_*` call or file I/O
- **Stub / skip** — add a one-liner that returns `false`/`0`/`FN_ERR_BAD_CMD`

Update the gap table entry when done.

## 5. Commit headers + implementation together

```bash
git add libs/fujinet-compat-amiga/include/ \
        libs/fujinet-compat-amiga/src/ \
        docs/audit-track1a-gap-table.md
git commit -m "Update compat layer to fujinet-lib <version/date>"
```
