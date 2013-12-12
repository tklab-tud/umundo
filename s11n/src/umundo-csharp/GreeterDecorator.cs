/**
 *  @file
 *  @brief      Decorator for the pure umundo Greeter
 *  @author     2012 Dirk Schnelle-Walka
 *  @copyright  Simplified BSD
 *
 *  @cond
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the FreeBSD license as published by the FreeBSD
 *  project.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  You should have received a copy of the FreeBSD license along with this
 *  program. If not, see <http://www.opensource.org/licenses/bsd-license>.
 *  @endcond
 */

using org.umundo.core;

namespace org.umundo.s11n
{
    class GreeterDecorator : Greeter
    {
        public ITypedGreeter Greeter { get; set; }
        public TypedPublisher TypedPublisher { get; set; }

        public override void welcome(Publisher publisher, SubscriberStub subStub)
        {
            if (Greeter != null)
            {
                Greeter.Welcome(TypedPublisher, subStub);
            }
        }

        public override void farewell(Publisher publisher, SubscriberStub subStub)
        {
            if (Greeter != null)
            {
                Greeter.Farewell(TypedPublisher, subStub);
            }
        }
    }
}
