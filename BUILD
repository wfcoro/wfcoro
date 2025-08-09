package(default_visibility = ["//visibility:public"])

cc_library(
	name = "common",
	includes = ["include"],
	hdrs = glob(["include/**/*.h"]),
	srcs = [
		"src/global.cc",
	],
	deps = [
		"@workflow//:common",
		"@marl//:marl",
	]
)

cc_library(
	name = "redis",
	includes = ["include"],
	hdrs = ["include/wfcoro/redis_client.h"],
	deps = [
		":common",
		"@workflow//:redis",
	]
)

cc_library(
	name = "mysql",
	includes = ["include"],
	hdrs = ["include/wfcoro/mysql_client.h"],
	deps = [
		":common",
		"@workflow//:mysql",
	]
)
