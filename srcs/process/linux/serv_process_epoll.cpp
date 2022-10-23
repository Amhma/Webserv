/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   serv_process_epoll.cpp                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: amahla <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/10/18 12:45:35 by amahla            #+#    #+#             */
/*   Updated: 2022/10/23 21:06:42 by amahla           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include <iostream>
# include <signal.h>
# include "webserv.h"

void	clearEpollProcess( std::vector<Server> & servers, std::vector<Client> & clients, t_epoll & epollVar )
{
	for (std::size_t i = 0; i < servers.size(); i++ )
	{
		epoll_ctl(epollVar.epollFd, EPOLL_CTL_DEL, servers[i].getSock(), NULL);
		close( servers[i].getSock() );
	}
	for (std::size_t i = 0; i < clients.size(); i++ )
	{
		epoll_ctl(epollVar.epollFd, EPOLL_CTL_DEL, clients[i].getSock(), NULL);
		close( clients[i].getSock() );
	}
	close( epollVar.epollFd );
}

void	appServ( std::vector<Server> & servers )
{
	std::vector<Client>	clients;
	t_epoll				epollVar;

	signal(SIGINT, &signal_handler);

	std::cout << std::endl << "\r" << YELLOW;
	std::cout << "-------------- Server is on ---------------" << std::endl;
	std::cout << SET << std::endl;
	try
	{
		servProcess( servers, clients, epollVar );
	}
	catch ( std::exception & e )
	{
		clearEpollProcess( servers, clients, epollVar );
		throw ( WebServException( e.what() ) ); 
	}
}
