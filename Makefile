.PHONY: all run editor run-editor clean rebuild

all:
	$(MAKE) -C game DEBUG=$(DEBUG)

run:
	$(MAKE) -C game run DEBUG=$(DEBUG)

editor:
	$(MAKE) -C game_editor DEBUG=$(DEBUG)

run-editor:
	$(MAKE) -C game_editor run DEBUG=$(DEBUG)

clean:
	$(MAKE) -C game clean
	$(MAKE) -C game_editor clean
	rm -f level-editor

rebuild: clean all editor
