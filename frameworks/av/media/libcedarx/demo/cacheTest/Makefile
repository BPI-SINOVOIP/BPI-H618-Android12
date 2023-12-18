all: awplayer-cache cmccplayer-cache
	@echo "Start test..."
	./awplayer-cache >/dev/null

CPPFLAGS := -g -D__STDC_FORMAT_MACROS

awplayer-src = cache.cpp ../../awplayer/cache.cpp
awplayer-include = -I../../awplayer/ -I$(PWD)

awplayer-cache: $(awplayer-src)
	g++ $(CPPFLAGS) -o $@ $(awplayer-src) $(awplayer-include) -lpthread

clean:
	@rm -f awplayer-cache

