package com.android.timezone.location.storage.tzs2range.read;

/**
 * An implementation of {@link SuffixTableBlock.SuffixTableBlockDelegate} for tables that are not
 * backed by real block data, i.e. have zero entries.
 */
final class UnpopulatedSuffixTableBlock implements SuffixTableBlock.SuffixTableBlockDelegate {

    private final int mPrefix;

    UnpopulatedSuffixTableBlock(int prefix) {
        mPrefix = prefix;
    }

    @Override
    public int getPrefix() {
        return mPrefix;
    }

    @Override
    public SuffixTableBlock.Entry findEntryByCellId(long cellId) {
        return null;
    }

    @Override
    public SuffixTableBlock.Entry findEntryByIndex(int i) {
        throw new IndexOutOfBoundsException("Unpopulated table");
    }

    @Override
    public int getEntryCount() {
        return 0;
    }

    @Override
    public int getTzIdSetBank() {
        throw new UnsupportedOperationException("Empty tables do not support this method.");
    }
}
