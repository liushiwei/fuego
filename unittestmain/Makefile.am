TESTS = fuego_unittest
check_PROGRAMS = $(TESTS)

fuego_unittest_SOURCES = \
../features/test/FeBasicFeaturesTest.cpp \
../features/test/FeFeatureWeightsTest.cpp \
../features/test/FeNestedPatternTest.cpp \
../features/test/FePatternTest.cpp \
../features/test/FePatternBaseTest.cpp \
../go/test/GoBoardTest.cpp \
../go/test/GoBoardSynchronizerTest.cpp \
../go/test/GoBoardUpdaterTest.cpp \
../go/test/GoBoardUtilTest.cpp \
../go/test/GoBookTest.cpp \
../go/test/GoEvalArrayTest.cpp \
../go/test/GoEyeUtilTest.cpp \
../go/test/GoGameTest.cpp \
../go/test/GoGtpCommandUtilTest.cpp \
../go/test/GoGtpEngineTest.cpp \
../go/test/GoKomiTest.cpp \
../go/test/GoLadderTest.cpp \
../go/test/GoOpeningKnowledgeTest.cpp \
../go/test/GoPatternBaseTest.cpp \
../go/test/GoPattern3x3Test.cpp \
../go/test/GoRegionTest.cpp \
../go/test/GoRegionBoardTest.cpp \
../go/test/GoSetupUtilTest.cpp \
../go/test/GoTimeControlTest.cpp \
../go/test/GoUtilTest.cpp \
../gouct/test/GoUctAdditiveKnowledgeMultipleTest.cpp \
../gouct/test/GoUctBoardTest.cpp \
../gouct/test/GoUctFeatureKnowledgeTest.cpp \
../gouct/test/GoUctFeaturesTest.cpp \
../gouct/test/GoUctKnowledgeTest.cpp \
../gouct/test/GoUctLadderKnowledgeTest.cpp \
../gouct/test/GoUctPatternsTest.cpp \
../gouct/test/GoUctUtilTest.cpp \
../gtpengine/test/GtpEngineTest.cpp \
../smartgame/test/SgArrayTest.cpp \
../smartgame/test/SgArrayListTest.cpp \
../smartgame/test/SgBlackWhiteTest.cpp \
../smartgame/test/SgBoardColorTest.cpp \
../smartgame/test/SgBoardConstTest.cpp \
../smartgame/test/SgBWArrayTest.cpp \
../smartgame/test/SgBWSetTest.cpp \
../smartgame/test/SgCmdLineOptTest.cpp \
../smartgame/test/SgConnCompIteratorTest.cpp \
../smartgame/test/SgEBWArrayTest.cpp \
../smartgame/test/SgEvaluatedMovesTest.cpp \
../smartgame/test/SgFastLogTest.cpp \
../smartgame/test/SgGameReaderTest.cpp \
../smartgame/test/SgGtpUtilTest.cpp \
../smartgame/test/SgHashTest.cpp \
../smartgame/test/SgMarkerTest.cpp \
../smartgame/test/SgMathTest.cpp \
../smartgame/test/SgMiaiMapTest.cpp \
../smartgame/test/SgMiaiStrategyTest.cpp \
../smartgame/test/SgNbIteratorTest.cpp \
../smartgame/test/SgNodeTest.cpp \
../smartgame/test/SgNodeUtilTest.cpp \
../smartgame/test/SgPointArrayTest.cpp \
../smartgame/test/SgPointTest.cpp \
../smartgame/test/SgPointSetTest.cpp \
../smartgame/test/SgPropTest.cpp \
../smartgame/test/SgRandomTest.cpp \
../smartgame/test/SgRectTest.cpp \
../smartgame/test/SgRestorerTest.cpp \
../smartgame/test/SgSearchTest.cpp \
../smartgame/test/SgSortedArrayTest.cpp \
../smartgame/test/SgSortedMovesTest.cpp \
../smartgame/test/SgStackTest.cpp \
../smartgame/test/SgStatisticsTest.cpp \
../smartgame/test/SgStringUtilTest.cpp \
../smartgame/test/SgSystemTest.cpp \
../smartgame/test/SgTimeControlTest.cpp \
../smartgame/test/SgTimeSettingsTest.cpp \
../smartgame/test/SgUctSearchTest.cpp \
../smartgame/test/SgUctTreeTest.cpp \
../smartgame/test/SgUctTreeUtilTest.cpp \
../smartgame/test/SgUctValueTest.cpp \
../smartgame/test/SgUtilTest.cpp \
../smartgame/test/SgVectorTest.cpp \
../smartgame/test/SgVectorUtilTest.cpp \
../smartgame/test/SgWriteTest.cpp \
../unittestmain/UnitTestMain.cpp

fuego_unittest_LDFLAGS = $(BOOST_LDFLAGS)

fuego_unittest_LDADD = \
../gouct/libfuego_gouct.a \
../simpleplayers/libfuego_simpleplayers.a \
../go/libfuego_go.a \
../features/libfuego_features.a \
../smartgame/libfuego_smartgame.a \
../gtpengine/libfuego_gtpengine.a \
$(BOOST_UNIT_TEST_FRAMEWORK_LIB) \
$(BOOST_FILESYSTEM_LIB) \
$(BOOST_SYSTEM_LIB) \
$(BOOST_THREAD_LIB)

fuego_unittest_DEPENDENCIES = \
../gouct/libfuego_gouct.a \
../simpleplayers/libfuego_simpleplayers.a \
../go/libfuego_go.a \
../features/libfuego_features.a \
../smartgame/libfuego_smartgame.a \
../gtpengine/libfuego_gtpengine.a

fuego_unittest_CPPFLAGS = \
$(BOOST_CPPFLAGS) \
-I@top_srcdir@/gtpengine \
-I@top_srcdir@/smartgame \
-I@top_srcdir@/go \
-I@top_srcdir@/features \
-I@top_srcdir@/simpleplayers \
-I@top_srcdir@/gouct

DISTCLEANFILES = *~
