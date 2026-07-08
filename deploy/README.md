# Deploying the signaling server

The signaling server is packaged as a container and published to the GitHub
Container Registry, so anyone can run a public OpenMesh signaling node.

## Image

- Dockerfile: [`docker/signaling.Dockerfile`](../docker/signaling.Dockerfile)
  (multi-stage; ~85 MB Debian-slim runtime with just the binary + libsodium).
- CI: [`.github/workflows/signaling-image.yml`](../.github/workflows/signaling-image.yml)
  builds **linux/amd64 + linux/arm64** on every push to `master` (or a `v*` tag)
  and pushes to `ghcr.io/<owner>/openmesh-signaling` with tags `latest`,
  `sha-<commit>`, and the git tag.

## Run it anywhere (Docker)

```sh
docker run -d --name openmesh-signaling \
  -p 4433:4433/udp \
  ghcr.io/ragnar-vallhala/openmesh-signaling:latest
```

Clients then use it:

```sh
om-chat --server <this-host-public-ip>:4433 --name alice
```

## Run it on the k3s cluster

```sh
kubectl apply -f deploy/k8s/signaling.yaml
kubectl -n openmesh get pods -o wide
```

This pins the pod to the public-IP node (`srv1237670`) and publishes
`hostPort 4433/udp`, so the server is reachable at `147.93.98.89:4433`.

**Make the image pullable.** GHCR packages are private by default. Either:

- make the package public — GitHub → your profile → Packages →
  `openmesh-signaling` → Package settings → change visibility to Public; or
- create a pull secret and reference it in the Deployment:
  ```sh
  kubectl -n openmesh create secret docker-registry ghcr-cred \
    --docker-server=ghcr.io \
    --docker-username=<github-user> \
    --docker-password=<a-PAT-with-read:packages>
  ```
  then uncomment `imagePullSecrets` in `signaling.yaml`.

**Open the firewall** for UDP 4433 on the host / cloud provider.

## Notes

- The server is stateless (an in-memory peer registry with TTLs), so a single
  replica is fine; it can be restarted freely.
- To pin a specific build instead of `latest`, set the image tag to
  `sha-<commit>` and drop `imagePullPolicy: Always`.
- NAT: the signaling server itself works across the internet, but two peers that
  are **both** behind NAT still can't exchange messages directly until the relay
  (FR-8) exists. Peers on public IPs / same LAN / with one public side work today.
