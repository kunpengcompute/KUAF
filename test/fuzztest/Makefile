PWD = $(shell pwd)
ROOT_DIR = $(PWD)/../../
GTEST_DOWNLOAD_DIR = $(ROOT_DIR)/test/gtest-download
GOOGLE_TEST_DIR = $(ROOT_DIR)/test/googletest

googletest:
	cd $(GTEST_DOWNLOAD_DIR);\
	sh download.sh;\
	mkdir -p $(GOOGLE_TEST_DIR)/build
	cd $(GOOGLE_TEST_DIR)/build/;\
	cmake $(GOOGLE_TEST_DIR);\
	cd $(PWD)/
	make -C $(GOOGLE_TEST_DIR)/build/ -j16

	@cp -u $(GOOGLE_TEST_DIR)/build/googlemock/*.a $(ROOT_DIR)/test/test_tool_bins/gtest/
	@cp -u $(GOOGLE_TEST_DIR)/build/googlemock/gtest/*.a $(ROOT_DIR)/test/test_tool_bins/gtest/

kuaflib:
	cd $(ROOT_DIR);\
	sh build.sh kuaf clean;\
	sh build.sh kuaf cover
	cd $(PWD)/

fuzz: googletest
	make kuaflib
	cmake -S $(ROOT_DIR)/test/fuzztest/ -B $(ROOT_DIR)/test/fuzztest/build
	make -C $(ROOT_DIR)/test/fuzztest/build -f Makefile -j16

rfuzz:
	make -C $(ROOT_DIR)/test/fuzztest/build -f Makefile run

clean:
	rm -rf build

.PHONY: all clean