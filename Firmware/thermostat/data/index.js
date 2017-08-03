angular
  .module('simpleThermometer', [])
  .controller('SimpleThermometerController', ['$scope', '$http', function($scope, $http) {
    $scope.status = 'connecting';
    $scope.state = {};
    tries = 0;
    (function update() {
      $http.get('/data', {timeout: 5000}).
      then(function success(response) {
        tries = 0;
        $scope.status = 'connected';
        $scope.state = response.data;
        setTimeout(update, 300);
      }, function error(response) {
        tries++;
        if(tries < 2)
          $scope.status = 'connecting';
        else {
          $scope.state = {temperature: 120.9483};
          $scope.status = 'disconnected';
        }
        setTimeout(update, 6000);
      });
    })();
  }]);
