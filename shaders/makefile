.PHONY: all clean

%.s.h :: %.shader
	xxd -i $< > $@

all: shaders.h
	
shaders.h: filter_fragment.s.h filter_vertex.s.h gobo_fragment.s.h gobo_vertex.s.h 
	cat *.s.h > $@

clean:
	$(RM) *.h

