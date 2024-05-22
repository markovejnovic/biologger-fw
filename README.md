# biologger-fw

## Building

1. Make sure you have zephyr installed before proceeding.
2. Clone the repo in your zephyr workspace
```bash
cd ~/zephyrproject
git clone git@github.com:markovejnovic/biologger-fw.git
cd biologger-fw
```
3. Build with west
```bash
west build -p auto -b biologger -- -DBOARD_ROOT="."
