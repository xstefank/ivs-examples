# Demo - Containerized C WebServer with external call for localization

1. Building the image

```bash
podman build -t webserver .
```

2. Running the webserver

```
podman run -it -p <PORT>:8080 webserver:latest
```

3. Calling the webserver (new terminal)

```bash
curl localhost:<PORT>
``````

## Example of ignored files

Run the following to see, README.md is ignored

```bash
podman exec -it -l ls
```

> Note: To kill the webserver from a different temrinal (killing last container)
>
> ```
> podman stop -l
> ```
