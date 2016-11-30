.PHONY: all clean shaders

all: filter_fragment.h filter_vertex.h gobo_fragment.h gobo_vertex.h

filter_fragment.h: filter.fragment
	xxd -i $< > $@

filter_vertex.h: filter.vertex
	xxd -i $< > $@

gobo_fragment.h: gobo.fragment
	xxd -i $< > $@

gobo_vertex.h: gobo.vertex
	xxd -i $< > $@

clean:
	$(RM) *.h

