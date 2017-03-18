var app=angular.module('VirtualSoc', ['ngRoute', 'ngIdle', 'ngAnimate', 'ngStorage']);
app.config(function($routeProvider, $locationProvider){
    $locationProvider.html5Mode(true);
    $locationProvider.hashPrefix('!');
    $routeProvider
        .when('/',{
            templateUrl:'snippets/login.html',
            controller:'loginController'
        })
        .when('/register', {
            templateUrl:'snippets/register.html',
            controller:'registerController'
        })
        .when('/user/:name',{
            templateUrl:'snippets/user.html',
            controller:'userController'
        })
        .when('/search',{
            templateUrl:'snippets/search.html',
            controller:'searchController'
        })
        .when('/messages/:name',{
            templateUrl: 'snippets/messages.html',
            controller: 'messagesController'
        })
        .otherwise({redirectTo:'/'});
});

app.config(function(IdleProvider, KeepaliveProvider) {
    IdleProvider.idle(900); // 15 min
    KeepaliveProvider.interval(80); 
    KeepaliveProvider.http('/api/heartbeat'); // URL that makes sure session is alive
});

app.run(function($rootScope, Idle) {
    Idle.watch();
    //$rootScope.$on('IdleStart', function() { /* Display modal warning or sth */ });
    //$rootScope.$on('IdleTimeout', function() { /* Logout user */ });
});


app.controller('loginController',login);
app.controller('registerController', register);
app.controller('userController', show_user_page);
app.controller('navigationController', navigation);
app.controller('searchController', search);
app.controller('messagesController', messagesController);


