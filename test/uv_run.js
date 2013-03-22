var i = 0; uv_run(0, function (option) { log(i+" : "+option); i++; if (i == 10) { uv_stop(); }});//UV_RUN_DEFAULT
//uv_run(function(option){log(option);});//UV_RUN_DEFAULT
//uv_run(1, function (option) { log(option); });//UV_RUN_ONCE
//uv_run(2, function (option) { log(option); });//UV_RUN_NOWAIT 不知道干嘛的