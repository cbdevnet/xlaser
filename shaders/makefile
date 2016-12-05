.PHONY: all clean
SHADERS = $(patsubst %.shader,%.s.h,$(notdir $(wildcard *.shader)))

%.s.h :: %.shader
	xxd -i $< > $@

all: shaders.h
	
shaders.h: $(SHADERS)
	cat *.s.h > $@

clean:
	$(RM) *.h
