/*
 * Copyright 2016, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.managedprovisioning.preprovisioning.terms;

import static android.view.View.TEXT_ALIGNMENT_TEXT_START;

import static com.android.internal.logging.nano.MetricsProto.MetricsEvent.PROVISIONING_TERMS_ACTIVITY_TIME_MS;
import static com.android.internal.util.Preconditions.checkNotNull;

import static com.google.android.setupdesign.util.ThemeHelper.shouldApplyExtendedPartnerConfig;

import static java.util.Objects.requireNonNull;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.view.ContextMenu;
import android.view.MenuItem;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.VisibleForTesting;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.lifecycle.ViewModelProvider;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.android.managedprovisioning.R;
import com.android.managedprovisioning.analytics.MetricsWriterFactory;
import com.android.managedprovisioning.analytics.ProvisioningAnalyticsTracker;
import com.android.managedprovisioning.common.AccessibilityContextMenuMaker;
import com.android.managedprovisioning.common.ManagedProvisioningSharedPreferences;
import com.android.managedprovisioning.common.SettingsFacade;
import com.android.managedprovisioning.common.SetupGlifLayoutActivity;
import com.android.managedprovisioning.common.StylerHelper;
import com.android.managedprovisioning.common.ThemeHelper;
import com.android.managedprovisioning.common.ThemeHelper.DefaultNightModeChecker;
import com.android.managedprovisioning.common.ThemeHelper.DefaultSetupWizardBridge;
import com.android.managedprovisioning.common.Utils;
import com.android.managedprovisioning.model.ProvisioningParams;
import com.android.managedprovisioning.preprovisioning.terms.TermsViewModel.TermsViewModelFactory;
import com.android.managedprovisioning.preprovisioning.terms.adapters.TermsListAdapter;

import java.util.List;
import java.util.function.BiFunction;

/**
 * Activity responsible for displaying the Terms screen
 */
public class TermsActivity extends SetupGlifLayoutActivity implements
        TermsListAdapter.TermsBridge {
    private final AccessibilityContextMenuMaker mContextMenuMaker;
    private final SettingsFacade mSettingsFacade;
    private ProvisioningAnalyticsTracker mProvisioningAnalyticsTracker;
    private final BiFunction<AppCompatActivity, ProvisioningParams, TermsViewModel>
            mViewModelFetcher;
    private TermsViewModel mViewModel;
    private final StylerHelper mStylerHelper;

    @SuppressWarnings("unused")
    public TermsActivity() {
        this(
                /* contextMenuMaker= */ null,
                new SettingsFacade(),
                new StylerHelper(),
                (activity, params) -> {
                    final TermsViewModelFactory factory =
                            new TermsViewModelFactory(activity.getApplication(), params);
                    return new ViewModelProvider(activity, factory).get(TermsViewModel.class);
                });
    }

    @VisibleForTesting
    TermsActivity(AccessibilityContextMenuMaker contextMenuMaker, SettingsFacade settingsFacade,
            StylerHelper stylerHelper,
            BiFunction<AppCompatActivity, ProvisioningParams, TermsViewModel> viewModelFetcher) {
        super(new Utils(), settingsFacade,
                new ThemeHelper(new DefaultNightModeChecker(), new DefaultSetupWizardBridge()));

        mContextMenuMaker =
                contextMenuMaker != null ? contextMenuMaker : new AccessibilityContextMenuMaker(
                        this);
        mSettingsFacade = requireNonNull(settingsFacade);
        mViewModelFetcher = requireNonNull(viewModelFetcher);
        mStylerHelper = requireNonNull(stylerHelper);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.terms_screen);
        setTitle(R.string.terms);

        ProvisioningParams params = checkNotNull(
                getIntent().getParcelableExtra(ProvisioningParams.EXTRA_PROVISIONING_PARAMS));
        mViewModel = mViewModelFetcher.apply(this, params);
        List<TermsDocument> terms = mViewModel.getTerms();

        initializeUiForHandhelds(terms);

        mProvisioningAnalyticsTracker = new ProvisioningAnalyticsTracker(
                MetricsWriterFactory.getMetricsWriter(this, mSettingsFacade),
                new ManagedProvisioningSharedPreferences(getApplicationContext()));
        mProvisioningAnalyticsTracker.logNumberOfTermsDisplayed(this, terms.size());
    }

    private void initializeUiForHandhelds(List<TermsDocument> terms) {
        setupHeader();
        setupRecyclerView();
        setupToolbar();
        setupTermsListForHandhelds(terms);
    }

    private void setupHeader() {
        if (!shouldApplyExtendedPartnerConfig(this)) {
            return;
        }
        TextView header = findViewById(R.id.header);
        header.setVisibility(View.VISIBLE);
        header.setText(R.string.terms);
        mStylerHelper.applyHeaderStyling(header,
                new LinearLayout.LayoutParams(header.getLayoutParams()));
        header.setTextAlignment(TEXT_ALIGNMENT_TEXT_START);
    }

    private void setupRecyclerView() {
        final RecyclerView recyclerView = findViewById(R.id.terms_container);
        if (recyclerView.getItemDecorationCount() > 0) {
            recyclerView.removeItemDecorationAt(/* index= */ 0);
        }
    }

    private void setupToolbar() {
        Toolbar toolbar = findViewById(R.id.toolbar);
        toolbar.setNavigationContentDescription(R.string.navigation_button_description);
        toolbar.setNavigationIcon(getDrawable(R.drawable.ic_arrow_back_24dp));
        toolbar.setNavigationOnClickListener(v ->
                getTransitionHelper().finishActivity(TermsActivity.this));
        if (!shouldApplyExtendedPartnerConfig(this)) {
            toolbar.setTitle(R.string.terms);
        }
    }

    private void setupTermsListForHandhelds(List<TermsDocument> terms) {
        RecyclerView recyclerView = findViewById(R.id.terms_container);
        recyclerView.setLayoutManager(new LinearLayoutManager(/* context= */ this));
        recyclerView.setAdapter(new TermsListAdapter(
                this,
                mViewModel.getGeneralDisclaimer(),
                terms,
                getLayoutInflater(),
                new AccessibilityContextMenuMaker(this),
                this,
                mUtils,
                mStylerHelper));
    }

    @Override
    public void onCreateContextMenu(ContextMenu menu, View v,
            ContextMenu.ContextMenuInfo menuInfo) {
        super.onCreateContextMenu(menu, v, menuInfo);
        if (v instanceof TextView) {
            mContextMenuMaker.populateMenuContent(menu, (TextView) v);
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == android.R.id.home) {
            onBackPressed();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public void onDestroy() {
        mProvisioningAnalyticsTracker.logNumberOfTermsRead(
                this, mViewModel.getNumberOfReadTerms());
        super.onDestroy();
    }

    @Override
    protected int getMetricsCategory() {
        return PROVISIONING_TERMS_ACTIVITY_TIME_MS;
    }

    @Override
    public boolean isTermExpanded(int groupPosition) {
        return mViewModel.isTermExpanded(groupPosition);
    }

    @Override
    public void onTermExpanded(int groupPosition, boolean expanded) {
        mViewModel.setTermExpanded(groupPosition, expanded);
    }

    @Override
    public void onLinkClicked(Intent intent) {
        getTransitionHelper().startActivityWithTransition(this, intent);
    }

    @Override
    protected boolean shouldSetupDynamicColors() {
        Context context = getApplicationContext();
        return !mSettingsFacade.isDeferredSetup(context)
                && !mSettingsFacade.isDuringSetupWizard(context);
    }
}
