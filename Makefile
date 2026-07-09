all:
	emcc -O3 src/c/ctp.c src/c/cantus.c src/c/chunks.c \
	  -s MODULARIZE=1 \
	  -s EXPORT_ES6=1 \
	  -s ENVIRONMENT=web \
	  -s EXPORTED_FUNCTIONS='["_set_bars","_set_mode","_generate_cantus","_get_cantus_value","_generate_ctp","_set_cantus","_get_ctp", "_get_solutions","_set_rule_param","_get_rule_param","_reset_rule_params"]' \
	  -s EXPORTED_RUNTIME_METHODS='["cwrap","getValue","HEAP32"]' \
	  -o src/cantus.js
