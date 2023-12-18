/*
 * Copyright (C) 2021 The Android Open Source Project
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

package com.android.bedstead.harrier;

import android.util.Log;

import androidx.annotation.Nullable;

import com.android.bedstead.harrier.annotations.CalledByHostDrivenTest;
import com.android.bedstead.harrier.annotations.enterprise.CannotSetPolicyTest;
import com.android.bedstead.harrier.annotations.enterprise.EnterprisePolicy;
import com.android.bedstead.harrier.annotations.enterprise.NegativePolicyTest;
import com.android.bedstead.harrier.annotations.enterprise.PositivePolicyTest;
import com.android.bedstead.harrier.annotations.meta.ParameterizedAnnotation;
import com.android.bedstead.harrier.annotations.meta.RepeatingAnnotation;
import com.android.bedstead.harrier.annotations.parameterized.IncludeNone;
import com.android.bedstead.nene.exceptions.NeneException;

import com.google.common.base.Objects;

import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runners.BlockJUnit4ClassRunner;
import org.junit.runners.model.FrameworkMethod;
import org.junit.runners.model.InitializationError;
import org.junit.runners.model.TestClass;

import java.lang.annotation.Annotation;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * A JUnit test runner for use with Bedstead.
 */
public final class BedsteadJUnit4 extends BlockJUnit4ClassRunner {

    private static final String BEDSTEAD_PACKAGE_NAME = "com.android.bedstead";

    // These are annotations which are not included indirectly
    private static final Set<String> sIgnoredAnnotationPackages = new HashSet<>();
    static {
        sIgnoredAnnotationPackages.add("java.lang.annotation");
        sIgnoredAnnotationPackages.add("com.android.bedstead.harrier.annotations.meta");
        sIgnoredAnnotationPackages.add("kotlin.*");
        sIgnoredAnnotationPackages.add("org.junit");
    }

    /**
     * {@link FrameworkMethod} subclass which allows modifying the test name and annotations.
     */
    public static final class BedsteadFrameworkMethod extends FrameworkMethod {

        private final Class<? extends Annotation> mParameterizedAnnotation;
        private final Map<Class<? extends Annotation>, Annotation> mAnnotationsMap =
                new HashMap<>();
        private Annotation[] mAnnotations;

        public BedsteadFrameworkMethod(Method method) {
            this(method, /* parameterizedAnnotation= */ null);
        }

        public BedsteadFrameworkMethod(Method method, Annotation parameterizedAnnotation) {
            super(method);
            this.mParameterizedAnnotation = (parameterizedAnnotation == null) ? null
                    : parameterizedAnnotation.annotationType();

            calculateAnnotations();
        }

        private void calculateAnnotations() {
            List<Annotation> annotations =
                    new ArrayList<>(Arrays.asList(getDeclaringClass().getAnnotations()));
            annotations.addAll(Arrays.asList(getMethod().getAnnotations()));

            parseEnterpriseAnnotations(annotations);

            resolveRecursiveAnnotations(annotations, mParameterizedAnnotation);

            this.mAnnotations = annotations.toArray(new Annotation[0]);
            for (Annotation annotation : annotations) {
                mAnnotationsMap.put(annotation.annotationType(), annotation);
            }
        }

        @Override
        public String getName() {
            if (mParameterizedAnnotation == null) {
                return super.getName();
            }
            return super.getName() + "[" + mParameterizedAnnotation.getSimpleName() + "]";
        }

        @Override
        public boolean equals(Object obj) {
            if (!super.equals(obj)) {
                return false;
            }

            if (!(obj instanceof BedsteadFrameworkMethod)) {
                return false;
            }

            BedsteadFrameworkMethod other = (BedsteadFrameworkMethod) obj;

            return Objects.equal(mParameterizedAnnotation, other.mParameterizedAnnotation);
        }

        @Override
        public Annotation[] getAnnotations() {
            return mAnnotations;
        }

        @Override
        public <T extends Annotation> T getAnnotation(Class<T> annotationType) {
            return (T) mAnnotationsMap.get(annotationType);
        }
    }

    /**
     * Resolve annotations recursively.
     *
     * @param parameterizedAnnotation The class of the parameterized annotation to expand, if any
     */
    public static void resolveRecursiveAnnotations(List<Annotation> annotations,
            @Nullable Class<? extends Annotation> parameterizedAnnotation) {
        int index = 0;
        while (index < annotations.size()) {
            Annotation annotation = annotations.get(index);
            annotations.remove(index);
            List<Annotation> replacementAnnotations =
                    getReplacementAnnotations(annotation, parameterizedAnnotation);
            annotations.addAll(index, replacementAnnotations);
            index += replacementAnnotations.size();
        }
    }

    private static List<Annotation> getReplacementAnnotations(Annotation annotation,
            @Nullable Class<? extends Annotation> parameterizedAnnotation) {
        List<Annotation> replacementAnnotations = new ArrayList<>();

        if (annotation.annotationType().getAnnotation(RepeatingAnnotation.class) != null) {
            try {
                Annotation[] annotations =
                        (Annotation[]) annotation.annotationType()
                                .getMethod("value").invoke(annotation);
                Collections.addAll(replacementAnnotations, annotations);
                return replacementAnnotations;
            } catch (IllegalAccessException | InvocationTargetException | NoSuchMethodException e) {
                throw new NeneException("Error expanding repeated annotations", e);
            }
        }

        if (annotation.annotationType().getAnnotation(ParameterizedAnnotation.class) != null
                && !annotation.annotationType().equals(parameterizedAnnotation)) {
            return replacementAnnotations;
        }

        for (Annotation indirectAnnotation : annotation.annotationType().getAnnotations()) {
            String annotationPackage = indirectAnnotation.annotationType().getPackage().getName();
            if (shouldSkipAnnotation(annotationPackage)) {
                continue;
            }

            replacementAnnotations.addAll(getReplacementAnnotations(
                    indirectAnnotation, parameterizedAnnotation));
        }

        replacementAnnotations.add(annotation);

        return replacementAnnotations;
    }

    private static boolean shouldSkipAnnotation(String annotationPackage) {
        for (String ignoredPackage : sIgnoredAnnotationPackages) {
            if (ignoredPackage.endsWith(".*")) {
                if (annotationPackage.startsWith(
                    ignoredPackage.substring(0, ignoredPackage.length() - 2))) {
                    return true;
                }
            } else if (annotationPackage.equals(ignoredPackage)) {
                return true;
            }
        }

        return false;
    }

    public BedsteadJUnit4(Class<?> testClass) throws InitializationError {
        super(testClass);
    }

    @Override
    protected List<FrameworkMethod> computeTestMethods() {
        TestClass testClass = getTestClass();

        List<FrameworkMethod> basicTests = new ArrayList<>();
        basicTests.addAll(testClass.getAnnotatedMethods(Test.class));
        basicTests.addAll(testClass.getAnnotatedMethods(CalledByHostDrivenTest.class));

        List<FrameworkMethod> modifiedTests = new ArrayList<>();

        for (FrameworkMethod m : basicTests) {
            Set<Annotation> parameterizedAnnotations = getParameterizedAnnotations(m);

            if (parameterizedAnnotations.isEmpty()) {
                // Unparameterized, just add the original
                modifiedTests.add(new BedsteadFrameworkMethod(m.getMethod()));
            }

            for (Annotation annotation : parameterizedAnnotations) {
                if (annotation.annotationType().equals(IncludeNone.class)) {
                    // Special case - does not generate a run
                    continue;
                }
                modifiedTests.add(
                        new BedsteadFrameworkMethod(m.getMethod(), annotation));
            }
        }

        sortMethodsByBedsteadAnnotations(modifiedTests);

        return modifiedTests;
    }

    /**
     * Sort methods so that methods with identical bedstead annotations are together.
     *
     * <p>This will also ensure that all tests methods which are not annotated for bedstead will
     * run before any tests which are annotated.
     */
    private void sortMethodsByBedsteadAnnotations(List<FrameworkMethod> modifiedTests) {
        List<Annotation> bedsteadAnnotationsSortedByMostCommon =
                bedsteadAnnotationsSortedByMostCommon(modifiedTests);

        modifiedTests.sort((o1, o2) -> {
            for (Annotation annotation : bedsteadAnnotationsSortedByMostCommon) {
                boolean o1HasAnnotation = o1.getAnnotation(annotation.annotationType()) != null;
                boolean o2HasAnnotation = o2.getAnnotation(annotation.annotationType()) != null;

                if (o1HasAnnotation && !o2HasAnnotation) {
                    // o1 goes to the end
                    return 1;
                } else if (o2HasAnnotation && !o1HasAnnotation) {
                    return -1;
                }
            }
            return 0;
        });
    }

    private List<Annotation> bedsteadAnnotationsSortedByMostCommon(List<FrameworkMethod> methods) {
        Map<Annotation, Integer> annotationCounts = countAnnotations(methods);
        List<Annotation> annotations = new ArrayList<>(annotationCounts.keySet());

        annotations.removeIf(
                annotation ->
                        !annotation.annotationType()
                                .getCanonicalName().contains(BEDSTEAD_PACKAGE_NAME));

        annotations.sort(Comparator.comparingInt(annotationCounts::get));
        Collections.reverse(annotations);

        return annotations;
    }

    private Map<Annotation, Integer> countAnnotations(List<FrameworkMethod> methods) {
        Map<Annotation, Integer> annotationCounts = new HashMap<>();

        for (FrameworkMethod method : methods) {
            for (Annotation annotation : method.getAnnotations()) {
                annotationCounts.put(
                        annotation, annotationCounts.getOrDefault(annotation, 0) + 1);
            }
        }

        return annotationCounts;
    }

    private Set<Annotation> getParameterizedAnnotations(FrameworkMethod method) {
        Set<Annotation> parameterizedAnnotations = new HashSet<>();
        List<Annotation> annotations = new ArrayList<>(Arrays.asList(method.getAnnotations()));

        // TODO(scottjonathan): We're doing this twice... does it matter?
        parseEnterpriseAnnotations(annotations);

        for (Annotation annotation : annotations) {
            if (annotation.annotationType().getAnnotation(ParameterizedAnnotation.class) != null) {
                parameterizedAnnotations.add(annotation);
            }
        }

        return parameterizedAnnotations;
    }

    /**
     * Parse enterprise-specific annotations.
     *
     * <p>To be used before general annotation processing.
     */
    private static void parseEnterpriseAnnotations(List<Annotation> annotations) {
        int index = 0;
        while (index < annotations.size()) {
            Annotation annotation = annotations.get(index);
            if (annotation instanceof PositivePolicyTest) {
                annotations.remove(index);
                Class<?> policy = ((PositivePolicyTest) annotation).policy();

                EnterprisePolicy enterprisePolicy =
                        policy.getAnnotation(EnterprisePolicy.class);
                List<Annotation> replacementAnnotations =
                        Policy.positiveStates(enterprisePolicy);

                annotations.addAll(index, replacementAnnotations);
                index += replacementAnnotations.size();
            } else if (annotation instanceof NegativePolicyTest) {
                annotations.remove(index);
                Class<?> policy = ((NegativePolicyTest) annotation).policy();

                EnterprisePolicy enterprisePolicy =
                        policy.getAnnotation(EnterprisePolicy.class);
                List<Annotation> replacementAnnotations =
                        Policy.negativeStates(enterprisePolicy);

                annotations.addAll(index, replacementAnnotations);
                index += replacementAnnotations.size();
            } else if (annotation instanceof CannotSetPolicyTest) {
                annotations.remove(index);
                Class<?> policy = ((CannotSetPolicyTest) annotation).policy();

                EnterprisePolicy enterprisePolicy =
                        policy.getAnnotation(EnterprisePolicy.class);
                List<Annotation> replacementAnnotations =
                        Policy.cannotSetPolicyStates(enterprisePolicy);

                annotations.addAll(index, replacementAnnotations);
                index += replacementAnnotations.size();
            } else {
                index++;
            }
        }
    }

    @Override
    protected List<TestRule> classRules() {
        List<TestRule> rules = super.classRules();

        for (TestRule rule : rules) {
            if (rule instanceof DeviceState) {
                DeviceState deviceState = (DeviceState) rule;

                deviceState.setSkipTestTeardown(true);
                deviceState.setUsingBedsteadJUnit4(true);

                break;
            }
        }

        return rules;
    }
}
