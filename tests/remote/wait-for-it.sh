#!/bin/bash
# wait-for-it.sh script
host="$1"
port="$2"
shift 2
cmd="$@"

until nc -z "$host" "$port"; do
    echo "Esperando al servidor en $host:$port..."
    sleep 1
done

exec $cmd
