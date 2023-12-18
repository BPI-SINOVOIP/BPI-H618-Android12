package com.android.eventlib;

/**
 * Service exposed to allow other packages to query logged events in this package.
 */
interface IQueryService {
    /**
     * Initialise a new query.
     *
     * <p>This method must be called before any other interaction with this service.
     *
     * <p>The {@code data} must contain a {@code QUERIER} key which contains a serialized instance
     * of {@code EventQuerier}.
     */
    void init(long id, in Bundle data);

    /**
     * Remote equivalent of {@code EventQuerier#get}.
     *
     * <p>The {@code data} must contain a {@code EARLIEST_LOG_TIME} key which contains a serialized
     * instance of {@code Instant}.
     *
     * <p>The return {@code Bundle} will contain a {@code EVENT} key with a serialized instance of
     * {@code Event}.
     */
    Bundle get(long id, in Bundle data);

    /**
     * Remote equivalent of {@code EventQuerier#get} which increments the count of skipped
     * results for calls to {@link #get}.
     *
     * <p>This should be used when the result from {@link #get} does not pass additional filters.
     *
     * <p>The {@code data} must contain a {@code EARLIEST_LOG_TIME} key which contains a serialized
     * instance of {@code Instant}.
     *
     * <p>The return {@code Bundle} will contain a {@code EVENT} key with a serialized instance of
     * {@code Event}.
     */
    Bundle getNext(long id, in Bundle data);

    /**
     * Remote equivalent of {@code EventQuerier#next}.
     *
     * <p>The {@code data} must contain a {@code EARLIEST_LOG_TIME} key which contains a serialized
     * instance of {@code Instant}.
     *
     * <p>The return {@code Bundle} will contain a {@code EVENT} key with a serialized instance of
     * {@code Event}.
     */
    Bundle next(long id, in Bundle data);

    /**
     * Remote equivalent of {@code EventQuerier#poll}.
     *
     * <p>The {@code data} must contain a {@code EARLIEST_LOG_TIME} key which contains a serialized
     * instance of {@code Instant}, and a {@code TIMEOUT} key which contains a serialized instance
     * of {@code Duration}.
     *
     * <p>The return {@code Bundle} will contain a {@code EVENT} key with a serialized instance of
     * {@code Event}.
     */
    Bundle poll(long id, in Bundle data);
}