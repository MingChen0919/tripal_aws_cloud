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
  dpm('less than 5 VPCs');
  $vpc = $ec2->createVpc([
            'CidrBlock' => '10.0.0.0/16',
            ]);
  $vpcArr = $vpc->toArray();
  dpm($vpcArr);
  $VpcId = $vpcArr['Vpc']['VpcId'];

  dpm($VpcId);

  ##==== create internetgateway =========
  $internetGateway = $ec2->createInternetGateway([]);
  $internetGatewayArr = $internetGateway->toArray();
  dpm($internetGatewayArr);
  $gatewayId = $internetGatewayArr['InternetGateway']['InternetGatewayId'];
  dpm("getway ID: $gatewayId");
  # attach internet gateway to the VPC
  $ec2->attachInternetGateway([
            'InternetGatewayId' => $gatewayId,
            'VpcId' => $VpcId,
          ]);

  ##===== Route table ===================
  $routeTable = $ec2->createRouteTable([
            'VpcId' => $VpcId,
          ]);
  $routeTableArr = $routeTable->toArray();
  $routeTableId = $routeTableArr['RouteTable']['RouteTableId'];
  # crate route
  $ec2->createRoute([
            'DestinationCidrBlock' => '0.0.0.0/0',
            'GatewayId' => $gatewayId,
            'RouteTableId' => $routeTableId,  
          ]);
  dpm("route table id: $routeTableId");

  ##==== create two public subnets ======
  # get available zones
  $availZones = $ec2->describeAvailabilityZones([]);
  $availZonesArr =  $availZones->toArray();

  dpm($availZonesArr);
  # public subnet 1
  $public_subnet_1 = $ec2->createSubnet([
            'AvailabilityZone' => $availZonesArr['AvailabilityZones'][0]['ZoneName'],
            'VpcId' => $VpcId,
            'CidrBlock' => '10.0.0.0/24',
            ]);
  $public_subnet_1_Arr = $public_subnet_1->toArray();
  $public_subnet_1_ID = $public_subnet_1_Arr['Subnet']['SubnetId'];
  $ec2->modifySubnetAttribute([
            'MapPublicIpOnLaunch' => [
              'Value' => true,
            ],
            'SubnetId' => $public_subnet_1_ID,
          ]);
  # public subnet 2
  $public_subnet_2 = $ec2->createSubnet([
            'AvailabilityZone' => $availZonesArr['AvailabilityZones'][0]['ZoneName'],
            'VpcId' => $VpcId,
            'CidrBlock' => '10.0.1.0/24',
            ]);
}

