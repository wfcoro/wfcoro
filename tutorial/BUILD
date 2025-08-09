cc_binary(
	name = "hello_world",
	srcs = ["tutorial-00-hello_world.cc"],
	deps = ["//:common"],
)

cc_binary(
	name = "http_server",
	srcs = ["tutorial-01-http_server.cc"],
	deps = ["//:common", "@workflow//:http"],
)

cc_binary(
	name = "parallel_sort",
	srcs = ["tutorial-02-parallel_sort.cc"],
	deps = ["//:common"],
)

cc_binary(
	name = "redis_cli",
	srcs = ["tutorial-03-redis_cli.cc"],
	deps = ["//:redis"],
)

cc_binary(
	name = "mysql_cli",
	srcs = ["tutorial-04-mysql_cli.cc"],
	deps = ["//:mysql"],
)

cc_binary(
	name = "future_wait",
	srcs = ["tutorial-05-future_wait.cc"],
	deps = ["//:common"],
)
