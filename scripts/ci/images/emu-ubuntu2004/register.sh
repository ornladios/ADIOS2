#!/x86_64/bin/sh

export PATH=/x86_64/bin

if [ ! -d /proc/sys/fs/binfmt_misc ]; then
    echo "No binfmt support in the kernel."
    echo "  Try: '/sbin/modprobe binfmt_misc' from the host"
    exit 1
fi


if [ ! -f /proc/sys/fs/binfmt_misc/register ]; then
    mount binfmt_misc -t binfmt_misc /proc/sys/fs/binfmt_misc
fi

RESET=0
TARGETS=""
while true
do
  case $1 in
    --reset)
      RESET=1
      ;;
    --targets)
      shift
      TARGETS="$1"
      ;;
    *)
      break
      ;;
  esac
  shift
done
    
if [ $RESET -eq 1 ]; then
    if [ -z "${TARGETS}" ]
    then
      find /proc/sys/fs/binfmt_misc -type f -name 'qemu-*' -exec sh -c 'echo "Resetting {}"; echo -1 > {}' \;
    else
      for TARGET in ${TARGETS}
      do
        find /proc/sys/fs/binfmt_misc -type f -name "qemu-${TARGET}*" -exec sh -c 'echo "Resetting {}"; echo -1 > {}' \;
      done
    fi
fi

/x86_64/bin/qemu-binfmt-conf.sh --qemu-suffix "-static" --qemu-path "/x86_64/bin" --qemu-targets "${TARGETS}" $@
