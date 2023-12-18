/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.car.telephony.common;

import android.net.Uri;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;

/** Represents query parameters. */
public class QueryParam {

    /** Creates a provider always returning the same query param instance. */
    public static Provider of(final QueryParam queryParam) {
        return () -> queryParam;
    }

    /**
     * An object capable of providing instances of {@link QueryParam}. It can be used in two
     * circumstances:
     * <ul>
     * <li>Return the same instance every time calling the getter.
     * <li>Return an updated instance or create a new instance every time calling the getter.
     *
     * @deprecated use {@link QueryBuilder}.
     */
    @Deprecated
    public interface Provider {

        /** Returns an instance of query params. */
        @Nullable
        QueryParam getQueryParam();
    }

    /** Used by {@link ObservableAsyncQuery#startQuery()} as query param. */
    final Uri mUri;
    /** Used by {@link ObservableAsyncQuery#startQuery()} as query param. */
    final String[] mProjection;
    /** Used by {@link ObservableAsyncQuery#startQuery()} as query param. */
    final String mSelection;
    /** Used by {@link ObservableAsyncQuery#startQuery()} as query param. */
    final String[] mSelectionArgs;
    /** Used by {@link ObservableAsyncQuery#startQuery()} as query param. */
    final String mOrderBy;
    /** Used by {@link ObservableAsyncQuery#startQuery()} to check query permission. */
    final String mPermission;

    public QueryParam(
            @NonNull Uri uri,
            @Nullable String[] projection,
            @Nullable String selection,
            @Nullable String[] selectionArgs,
            @Nullable String orderBy,
            @NonNull String permission) {
        mUri = uri;
        mProjection = projection;
        mSelection = selection;
        mSelectionArgs = selectionArgs;
        mOrderBy = orderBy;
        mPermission = permission;
    }

    /**
     * Builds the {@link QueryParam}.
     */
    public static class QueryBuilder {
        public static final String AND = "AND";
        public static final String OR = "OR";

        static final String ORDER_ASC = "ASC";
        static final String ORDER_DESC = "DESC";

        private final Uri mUri;

        private String mPermission;

        private Condition mWhere = Condition.emptyCondition();
        private final List<String> mOrderBy = new ArrayList<>();
        private final Set<String> mProjectionColumns = new HashSet<>();

        /**
         *
         * @param contentUri the content uri for querying.
         */
        public QueryBuilder(Uri contentUri) {
            mUri = contentUri;
        }

        /**
         * Specify the projection column. Calling this function multiple times to add more columns
         * to the projection. When the same column is added multiple times, only one column will
         * be remembered.
         */
        public QueryBuilder project(String columnName) {
            mProjectionColumns.add(columnName);
            return this;
        }

        /**
         * Selects all columns for projection. This function call will override all previous
         * {@link #project(String)} call. Calling this function is optional. By default, all
         * columns will be selected.
         */
        public QueryBuilder projectAll() {
            mProjectionColumns.clear();
            return this;
        }

        /**
         * Specify the select condition.
         *
         * @see Condition
         */
        public QueryBuilder where(@NonNull Condition condition) {
            mWhere = condition;
            return this;
        }

        private QueryBuilder orderBy(String column, String order) {
            mOrderBy.add(column + " " + order);
            return this;
        }

        /**
         * Sorts the query result in an ascending order based on the given column.
         * Calling this function multiple times will add additional orders.
         * The order of calling this method matters.
         * @param column The column to be order by.
         */
        public QueryBuilder orderAscBy(String column) {
            return orderBy(column, ORDER_ASC);
        }

        /**
         * Sorts the query result in an descending order based on the given column.
         * Calling this function multiple times will add additional orders.
         * The order of calling this method matters.
         * @param column The column to be order by.
         */
        public QueryBuilder orderDescBy(String column) {
            return orderBy(column, ORDER_DESC);
        }

        /**
         * Specify the column and order all together.
         * @param orderBy Should follow "{column} {order}" pattern.
         */
        public QueryBuilder orderBy(String orderBy) {
            mOrderBy.add(orderBy);
            return this;
        }

        /**Specifies the permission needed for this query.*/
        public QueryBuilder checkPermission(@NonNull String permission) {
            mPermission = permission;
            return this;
        }
        /**
         * Builds the selection condition. Use {@link #is(String, String, Object)} to create an
         * initial condition.
         */
        public static class Condition {
            private String mSelection = "";
            private List<String> mArgs = new ArrayList<>();

            private Condition() {}

            private static Condition emptyCondition() {
                Condition condition = new Condition();
                condition.mSelection = null;
                return condition;
            }

            /**
             * Creates the inital condition.
             */
            public static Condition is(String columnName, String operator, @NonNull Object value) {
                Condition condition = new Condition();
                condition.mSelection = condition.toSelectionString(columnName, operator);
                condition.mArgs.add(value.toString());
                return condition;
            }

            /**
             * Appends an "AND" condition to the existing conditions.
             */
            public Condition and(String columnName, String operator, @NonNull Object value) {
                mSelection += wrapWithSpaces(AND) + toSelectionString(columnName, operator);
                mArgs.add(value.toString());
                return this;
            }

            /**
             * Appends an "OR" condition to the existing conditions.
             */
            public Condition or(String columnName, String operator, @NonNull Object value) {
                mSelection += wrapWithSpaces(OR) + toSelectionString(columnName, operator);
                mArgs.add(value.toString());
                return this;
            }

            public String getSelection() {
                return mSelection;
            }

            public String[] getWhereArgs() {
                return mArgs.stream().toArray(String[]::new);
            }

            static String toSelectionString(String columnName, String op) {
                return columnName + " " + op + " ?";
            }

            static String wrapWithSpaces(String s) {
                return " " + s + " ";
            }
        }

        /**
         * Builds the QueryParam.
         */
        public QueryParam toQueryParam() {
            String[] projection = mProjectionColumns.isEmpty()
                    ? null
                    : mProjectionColumns.stream().toArray(String[]::new);
            String orderBy = mOrderBy.stream().collect(Collectors.joining(","));

            return new QueryParam(
                    mUri,
                    projection,
                    mWhere.getSelection(),
                    mWhere.getWhereArgs(),
                    orderBy,
                    mPermission);
        }
    }
}
