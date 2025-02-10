CFLAGS := -g -Wall -Wextra -Wpedantic -Iinclude


all: lib demo

lib: lib/libwitui.a

demo: demo/out/simple_demo.out demo/out/station_schedule.out


lib/libwitui.a: obj/handle_input.o obj/rendering.o obj/tui.o obj/utility.o
	@mkdir -p $(@D) # Create lib/ if needed
	ar rcs $@ $^   # Bundle al target-inputs into an archive


DEMO_DEPS := lib/libwitui.a include/wi_data.h include/wi_functions.h

demo/out/simple_demo.out: $(DEMO_DEPS) demo/simple_demo.c
	@mkdir -p $(@D) # Create lib/ if needed
	gcc $(CFLAGS) demo/simple_demo.c -o $@ -Llib -lwitui


demo/out/station_schedule.out: $(DEMO_DEPS) demo/station_schedule.c
	@mkdir -p $(@D) # Create lib/ if needed
	gcc $(CFLAGS) demo/station_schedule.c -o $@ -Llib -lwitui


COMMON := include/wiAssert.h include/wi_internals.h include/wi_functions.h

obj/handle_input.o: $(COMMON) src/handle_input.c
	@mkdir -p $(@D) # Create lib/ if needed
	gcc $(CFLAGS) -c src/handle_input.c -o $@

obj/rendering.o: $(COMMON) src/rendering.c
	@mkdir -p $(@D) # Create lib/ if needed
	gcc $(CFLAGS) -c src/rendering.c -o $@

obj/tui.o: $(COMMON) include/wi_data.h src/tui.c
	@mkdir -p $(@D) # Create lib/ if needed
	gcc $(CFLAGS) -c src/tui.c -o $@

obj/utility.o: $(COMMON) include/wi_data.h src/utility.c
	@mkdir -p $(@D) # Create lib/ if needed
	gcc $(CFLAGS) -c src/utility.c -o $@


clean:
	-rm -r demo/out/
	-rm -r obj/
	-rm -r lib/

.PHONY: all clean lib demo
