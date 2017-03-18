login.$inject = ['$scope', '$location', '$http', '$sessionStorage'];
function login($scope, $location, $http, $sessionStorage) {
    changeBackgroundWithImage();
    $scope.$storage = $sessionStorage;

    $scope.location = $location;
    $scope.message = ($location.search()).message;

    $scope.login = function (user) {
        if (Object.keys(user).length) {
            var req = {
                method: "POST",
                url: "/login",
                headers: {
                    "Content-Type": "*/*",
                    "Connection": "keep-alive"
                },
                data: user
            };
            $http(req)
                .then(function (response) {
                    console.log("succes");
                    console.log(response);
                    $scope.message = response.data.message;
                    $location.url(response.data.uri);
                    $scope.$storage.username = user.username;
                }, function (response) {
                    console.log("fail");
                    console.log(response);
                    $scope.message = response.data.message;
                });
        }
    };


}

register.$inject = ['$scope', '$location', '$http'];
function register($scope, $location, $http) {
    changeBackgroundWithImage();
    $scope.location = $location;
    $scope.message = ($location.search()).message;

    $scope.register = function (user) {
        if (Object.keys(user).length) {
            var req = {
                method: "POST",
                url: "/register",
                headers: {
                    "Content-Type": "*/*",
                    "Connection": "keep-alive"
                },
                data: user
            };
            $http(req)
                .then(function (response) {
                    console.log("succes");
                    console.log(response);
                    $scope.message = response.data.message;
                    $location.url(response.data.uri);
                }, function (response) {
                    console.log("fail");
                    console.log(response);
                    $scope.message = response.data.message;
                });
        }
    };

}
show_user_page.$inject = ['$scope', '$location', '$http', '$route', '$routeParams', '$sessionStorage'];
function show_user_page($scope, $location, $http, $route, $routeParams, $sessionStorage) {
    changeBackgroundToStock();
    $scope.$storage = $sessionStorage;
    $scope.username = $routeParams.name;
    $scope.posts = {};
    $scope.post = function (user) {
        var today = new Date();
        var dd = today.getDate();
        var mm = today.getMonth() + 1;
        var yyyy = today.getFullYear();

        if (dd < 10) {
            dd = '0' + dd
        }

        if (mm < 10) {
            mm = '0' + mm
        }
        today = mm + '/' + dd + '/' + yyyy + " " + today.getHours() + ":" + today.getMinutes();
        user.date = today;
        console.log($scope.posts);
        if (Object.keys(user).length == 3) {
            var send_post = {
                method: "POST",
                url: "/post",
                headers: {
                    "Content-Type": "*/*",
                    "Connection": "keep-alive"
                },
                data: user
            };
            $http(send_post)
                .then(function (response) {
                    console.log("success, the post was succesfuly posted");
                    console.log(response);
                    var new_post = {};
                    new_post.content = user.content;
                    new_post.date = user.date;
                    $scope.posts.push(new_post);
                    user.visibility = "visibility";
                    user.content = "";
                    console.log($scope.posts);
                }, function (response) {
                    console.log("fail");
                    console.log(response);
                });

        }
    };

    $scope.sendRelationship = function(user){
        user.username = $scope.username;
        console.log(user);
        if (Object.keys(user).length == 2) {
            var send_rel = {
                method: "POST",
                url: "/relationship",
                headers: {
                    "Content-Type": "*/*",
                    "Connection": "keep-alive"
                },
                data: user
            };
            $http(send_rel)
                .then(function (response) {
                    console.log("success, the relationship was succesfuly added");
                    console.log(response);
                }, function (response) {
                    console.log("fail");
                    console.log(response);
                });

        }
    };

    (function() {
        var req = {
            method: "POST",
            url: "/user/" + $routeParams.name,
            headers: {
                "Content-Type": "*/*",
                "Connection": "keep-alive"
            },
            data: "{\"username\":\"" + $routeParams.name + "\"}"
        };
        $http(req)
            .then(function (response) {
                console.log(response);
                $scope.posts = response.data;
            }, function (response) {
                console.log("fail");
                console.log(response);
            });
    })();


}

navigation.$inject = ['$scope', '$location', '$http', '$route', '$routeParams', '$sessionStorage'];
function navigation($scope, $location, $http, $route, $routeParams, $sessionStorage) {
    changeBackgroundToStock();
    $scope.$storage = $sessionStorage;
    $scope.search_results = {};
    $scope.search = function (search_user) {
        if (Object.keys(search_user).length) {
            var req = {
                method: "POST",
                url: "/search",
                headers: {
                    "Content-Type": "*/*",
                    "Connection": "keep-alive"
                },
                data: search_user
            };
            $http(req)
                .then(function (response) {
                    console.log("succes");
                    console.log(response);
                    $scope.search_results = response.data;
                    $location.url("/search");
                }, function (response) {
                    console.log("fail");
                    console.log(response);
                    $scope.message = response.data.message;
                });
        }
    }
}

search.$inject = ['$scope', '$location', '$http', '$route', '$routeParams'];
function search($scope, $location, $http, $route, $routeParams) {
    changeBackgroundToStock();
}

function changeBackgroundWithImage(){
    $("body").addClass("starBackground");
}

function changeBackgroundToStock(){
    $("body").removeClass("starBackground");
}
messagesController.$inject = ['$scope', '$location', '$http', '$route', '$routeParams', '$sessionStorage'];
function messagesController($scope, $location, $http, $route, $routeParams, $sessionStorage){
    $scope.$storage = $sessionStorage;
    $scope.posts = {};
    $scope.sendMessage = function(user){
        var today = new Date();
        var dd = today.getDate();
        var mm = today.getMonth() + 1;
        var yyyy = today.getFullYear();

        if (dd < 10) {
            dd = '0' + dd
        }

        if (mm < 10) {
            mm = '0' + mm
        }
        today = mm + '/' + dd + '/' + yyyy + " " + today.getHours() + ":" + today.getMinutes();
        user.date = today;
        user.username = $routeParams.name;
        if (Object.keys(user).length == 3) {
            var send_post = {
                method: "POST",
                url: "/messages/send",
                headers: {
                    "Content-Type": "*/*",
                    "Connection": "keep-alive"
                },
                data: user
            };
            $http(send_post)
                .then(function (response) {
                    console.log("success, the message was succesfuly posted");
                    console.log(response);
                    var new_post = {};
                    new_post.content = user.content;
                    new_post.date = user.date;
                    $scope.posts.push(new_post);
                    user.content = "";
                    console.log($scope.posts);
                }, function (response) {
                    console.log("fail");
                    console.log(response);
                });

        }
    };


    (function() {
        var req = {
            method: "POST",
            url: "/messages/receive",
            headers: {
                "Content-Type": "*/*",
                "Connection": "keep-alive"
            },
            data: "{\"username\":\"" + $routeParams.name + "\"}"
        };
        $http(req)
            .then(function (response) {
                console.log(response);
                $scope.posts = response.data;
            }, function (response) {
                console.log("fail");
                console.log(response);
            });
    })();
}
//user.visibility = "visibility";