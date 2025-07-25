include(ExportDirectory)
set(BASE_SOURCE_FILES
	circle-buffer.c
	configuration.c
	crc32.c
	formatting.c
	gbk-table.c
	hash.c
	md5.c
	sha1.c
	string.c
	table.c
	vector.c
	vfs.c)

set(SOURCE_FILES
	${BASE_SOURCE_FILES}
	audio-buffer.c
	audio-resampler.c
	convolve.c
	elf-read.c
	geometry.c
	image.c
	image/export.c
	image/font.c
	image/png-io.c
	interpolator.c
	patch.c
	patch-fast.c
	patch-ips.c
	patch-ups.c
	ring-fifo.c
	sfo.c
	text-codec.c)

set(GUI_FILES
	gui.c
	gui/file-select.c
	gui/font.c
	gui/font-metrics.c
	gui/menu.c)

set(TEST_FILES
	test/circle-buffer.c
	test/color.c
	test/geometry.c
	test/hash.c
	test/image.c
	test/sfo.c
	test/string-parser.c
	test/string-utf8.c
	test/table.c
	test/text-codec.c
	test/vfs.c)

if(NOT DEFINED OS_SRC)
	set(OS_FILES memory.c)
	export_directory(OS OS_FILES)
endif()

if(USE_LIBMOBILE)
	list(APPEND SOURCE_FILES mobile.c)
endif()

source_group("Utilities" FILES ${SOURCE_FILES})
source_group("GUI source" FILES ${GUI_FILES})
source_group("Utilities tests" FILES ${TEST_FILES})

export_directory(UTIL_BASE BASE_SOURCE_FILES)
export_directory(UTIL SOURCE_FILES)
export_directory(GUI GUI_FILES)
export_directory(UTIL_TEST TEST_FILES)
