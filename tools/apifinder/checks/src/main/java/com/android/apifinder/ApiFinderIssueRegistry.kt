package com.android.apifinder

import com.android.tools.lint.client.api.IssueRegistry
import com.google.auto.service.AutoService

@AutoService(IssueRegistry::class)
@Suppress("UnstableApiUsage")
class ApiFinderIssueRegistry : IssueRegistry() {
    override val api: Int
        get() = 7
    override val issues = listOf(ApiFinderDetector.ISSUE)
}
