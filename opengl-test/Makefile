output = triangles \
		 tri0 \
		drm-test \
		modeset-plane-test \

CFLAGS = -g -DGL_GLEXT_PROTOTYPES
LDFLAGS = -lglfw -lGLESv2 -lEGL -lX11 -ldrm -I /usr/include/glad/

all: $(output)

$(output): %: %.c
	gcc -o $@ $< $(CFLAGS) $(LDFLAGS)

.PHONY: clean

clean:
	rm -f $(output)
