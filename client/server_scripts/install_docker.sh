if which apt-get > /dev/null 2>&1; then pm=$(which apt-get); docker_pkg="docker.io"; dist="debian";\
elif which dnf > /dev/null 2>&1; then pm=$(which dnf); docker_pkg="docker"; dist="fedora";\
elif which yum > /dev/null 2>&1; then pm=$(which yum); docker_pkg="docker"; dist="centos";\
else echo "Packet manager not found"; exit 1; fi;\
echo "Dist: $dist, Packet manager: $pm, Docker: $docker_pkg";\
if [[ "$dist" = "debian" ]]; then export DEBIAN_FRONTEND=noninteractive; fi;\
if [[ ! -f "/usr/bin/sudo" ]]; then $pm update -yq; $pm install -yq sudo; fi;\
if ! command -v sudo &> /dev/null; then $pm update -yq; $pm install -yq sudo; fi\
if ! command -v sudo &> /dev/null; then $pm install -yq psmisc; fi\
if ! command -v docker &> /dev/null; then $pm update -yq; $pm install -yq $docker_pkg;\
  if [[ "$dist" = "fedora" || "$dist" = "debian" ]]; then sudo systemctl enable docker && sudo systemctl start docker; fi;\
fi;\
if [[ "$dist" = "debian" ]]; then;\
  docker_service=$(systemctl list-units --full --all | grep docker.service | grep -v inactive | grep -v dead | grep -v failed);\
  if [[ -z "$docker_service" ]]; then sudo $pm update -yq; sudo $pm install -yq curl $docker_pkg; fi;\
  sleep 3 && sudo systemctl start docker && sleep 3;\
fi;\
if ! command -v sudo &> /dev/null; then echo "Failed to install Docker";exit 1;fi;\
$pm install -yq lsof;\
docker --version

