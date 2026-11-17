# GLAD 1 OpenGL loader

Generated with the official GLAD 0.1.36 generator from
https://github.com/Dav1dde/glad/tree/1ecd45775d96f35170458e6b148eb0708967e402.

Command:

```sh
python -m glad --profile core --api gl=3.3 --generator c --extensions '' --out-path third_party/glad --reproducible
```

The generator uses its bundled Khronos specification. Windows users need neither
Python nor a GLAD website download. The generated loader is infrastructure, not
the project's rendering or gameplay implementation. Preserve LICENSE and the
copyright notices embedded in the generated headers and source.
