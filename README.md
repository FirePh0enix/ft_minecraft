# ft_minecraft

To compile the projet:

```bash
cmake -B /tmp/ftmc -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

- `-B /tmp/ftmc` because there is not enough space in the session
- `-G Ninja` because it would take 10 minutes to compile otherwise
- `-DCMAKE_BUILD_TYPE=RelWithDebInfo` for performance reason
