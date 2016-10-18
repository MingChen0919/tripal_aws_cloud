<?php

## Create ec2 client
use Aws\Credentials\CredentialProvider;
use Aws\Ec2\Ec2Client;

$provider = CredentialProvider::ini();

$ec2 = new Aws\Ec2\Ec2Client([
          'version' => 'latest',
          'region' => 'us-east-1',
          'credentials' => $provider
          ]);


$describeVpcs = $ec2->describeVpcs([]);
$describeVpcsArr = $describeVpcs->toArray();

dpm($describeVpcsArr);

if(count($describeVpcsArr)> 4){
  break;
} else {

}

