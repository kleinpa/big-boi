angular
  .module('bigboi', [])
  .controller('BigBoiController', ['$scope', '$http', function($scope, $http) {
    $scope.status = 'connecting';
    $scope.state = {};
    tries = 0;
    (function update() {
      $http.get('/data?thermometers=1', {timeout: 5000}).
      then(function success(response) {
        tries = 0;
        $scope.status = 'connected';
        $scope.state = response.data;
        setTimeout(update, 1000);
      }, function error(response) {
        tries++;
        if(tries < 2)
          $scope.status = 'connecting';
        else {
          $scope.state = {};
          $scope.status = 'disconnected';
        }
        setTimeout(update, 6000);
      });
    })();
    $scope.sendCompressorThermometer = function(address){
      if($scope.status == 'connected')
        $scope.state.compressorThermometer = address;
      $http.post('/data', "compressorThermometer=" + address);
    };
    $scope.sendFridgeThermometer = function(address){
      if($scope.status == 'connected')
        $scope.state.fridgeThermometer = address;
      $http.post('/data', "fridgeThermometer=" + address);
    };
    $scope.sendSetpoint = function(setpoint){
      if($scope.status == 'connected')
        $scope.state.setpoint = setpoint;
      $http.post('/data', "setpoint=" + setpoint);
    };
    $scope.sendEpsilon = function(epsilon){
      if($scope.status == 'connected')
        $scope.state.epsilon = epsilon;
      $http.post('/data', "epsilon=" + epsilon);
    };
    $scope.sendCompressorLimit = function(compressorLimit){
      if($scope.status == 'connected')
        $scope.state.compressorLimit = compressorLimit;
      $http.post('/data', "compressorLimit=" + compressorLimit);
    };
  }]);
