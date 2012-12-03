use ConfigHelper;

my $cfg = new ConfigHelper;

$cfg->set_debug_tags('http_hdrs|ldap.*');
$cfg->add_config('plugins.config');
$cfg->add_config_line ('vscan.so');

$cfg->add_config('storage.new');
$cfg->add_config_line('. 120000000');

$cfg->add_config('filter.config');
$cfg->add_config_line ('dest_domain=. action=ldap');

# change records.config
$cfg->set_record ('proxy.config.cache.storage_filename', 'storage.new');
$cfg->add_record ('INT', 'dummy.record', '12');

#print
print $cfg->output;
