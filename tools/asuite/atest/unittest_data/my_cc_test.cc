INSTANTIATE_TEST_SUITE_P( Instantiation1, MyInstantClass1,
    testing::Combine(testing::Values(Options::Language::CPP, Options::Language::JAVA,
                                     Options::Language::NDK, Options::Language::RUST),
                     testing::ValuesIn(kTypeParams)),
    [](const testing::TestParamInfo<std::tuple<Options::Language, TypeParam>>& info) {
      return Options::LanguageToString(std::get<0>(info.param)) + "_" +
             std::get<1>(info.param).kind;
    });

INSTANTIATE_TEST_CASE_P(Instantiation2,
    MyInstantClass2,
    testing::Combine(testing::Values(Options::Language::CPP, Options::Language::JAVA,
                                     Options::Language::NDK, Options::Language::RUST),
                     testing::ValuesIn(kTypeParams)),
    [](const testing::TestParamInfo<std::tuple<Options::Language, TypeParam>>& info) {
      return Options::LanguageToString(std::get<0>(info.param)) + "_" +
             std::get<1>(info.param).kind;
    });

INSTANTIATE_TEST_SUITE_P(
    Instantiation3, MyInstantClass1 ,
    testing::Combine(testing::Values(Options::Language::CPP, Options::Language::JAVA,
                                     Options::Language::NDK, Options::Language::RUST),
                     testing::ValuesIn(kTypeParams)),
    [](const testing::TestParamInfo<std::tuple<Options::Language, TypeParam>>& info) {
      return Options::LanguageToString(std::get<0>(info.param)) + "_" +
             std::get<1>(info.param).kind;
    });


INSTANTIATE_TEST_CASE_P(
    Instantiation4,
    MyInstantClass3,
    testing::Combine(testing::Values(Options::Language::CPP, Options::Language::JAVA,
                                     Options::Language::NDK, Options::Language::RUST),
                     testing::ValuesIn(kTypeParams)),
    [](const testing::TestParamInfo<std::tuple<Options::Language, TypeParam>>& info) {
      return Options::LanguageToString(std::get<0>(info.param)) + "_" +
             std::get<1>(info.param).kind;
    });
    
TEST_P( MyClass1, Method1) {
  Run("List<{}>", kListSupportExpectations);
}

TEST_F(
MyClass1, 
Method2) {
  Run("List<{}>", kListSupportExpectations);
}

TEST_P(MyClass2, 
       Method3) {
  Run("List<{}>", kListSupportExpectations);
}

TEST_F(MyClass3, Method2) {
  Run("List<{}>", kListSupportExpectations);
}

TEST(MyClass4, Method5) {
  Run("List<{}>", kListSupportExpectations);
}

TEST(MyClass5, Method5) {
  Run("List<{}>", kListSupportExpectations);
}

INSTANTIATE_TYPED_TEST_CASE_P(Instantiation5, MyInstantTypeClass1, IntTypes);

INSTANTIATE_TYPED_TEST_SUITE_P(Instantiation6, MyInstantTypeClass2, IntTypes);

