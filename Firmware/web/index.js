angular
  .module('bigboi', [])
  .controller('BigBoiController', ['$scope', '$http', function($scope, $http) {
    $scope.status = 'connecting';
    $scope.state = {};
    tries = 0;
    (function update() {
      $http.get('/data', {timeout: 3000}).
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
  }]);
